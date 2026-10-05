// AssetCache: loads every model and every texture once and hands out stable references.
// See docs/modules/assets/asset-cache.md
#include "assets/AssetCache.hpp"

#include "assets/ImageLoader.hpp"
#include "assets/ObjLoader.hpp"
#include "core/Log.hpp"
#include "core/Paths.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace assets {

namespace {

// The white stand-in texture: one pixel of 3 bytes (red, green, blue), all at the maximum.
constexpr int WHITE_TEXTURE_SIZE = 1;
constexpr int WHITE_TEXTURE_CHANNELS = 3;
constexpr unsigned char FULL_BRIGHTNESS = 255;
constexpr std::array<unsigned char, WHITE_TEXTURE_CHANNELS> WHITE_PIXEL = {
    FULL_BRIGHTNESS, FULL_BRIGHTNESS, FULL_BRIGHTNESS};

// Anisotropy level 1 means "one sample": no anisotropic filtering.
constexpr float NO_ANISOTROPY = 1.0F;

// A triangle is three indices.
constexpr std::size_t INDICES_PER_TRIANGLE = 3;

// The form of a path that is used as the key: without "." and ".." steps, so that two
// spellings of the same file (models/../textures/a.png and textures/a.png) are one entry.
// lexically_normal works on the text of the path and does not ask the file system.
std::filesystem::path cacheKey(const std::filesystem::path& path) {
    return path.lexically_normal();
}

// The material called name, or nullptr when the model has none with that name.
const ObjMaterial* findMaterial(const ObjModel& model, const std::string& name) {
    for (const ObjMaterial& material : model.materials) {
        if (material.name == name) {
            return &material;
        }
    }
    return nullptr;
}

} // namespace

AssetCache::AssetCache()
    : m_whiteTexture(WHITE_TEXTURE_SIZE, WHITE_TEXTURE_SIZE, WHITE_TEXTURE_CHANNELS,
                     WHITE_PIXEL.data()) {}

const LoadedModel* AssetCache::model(const std::filesystem::path& path) {
    const std::filesystem::path key = cacheKey(path);

    // Loaded before: hand out the same object. A game has a handful of models, so
    // looking through all of them is fast enough and needs no second data structure.
    for (const LoadedModel& loaded : m_models) {
        if (loaded.path == key) {
            return &loaded;
        }
    }
    // Failed before: the error is already in the log, do not read the file again.
    if (hasFailed(key)) {
        return nullptr;
    }

    // loadObj logs its own error.
    ObjModel source;
    std::string error;
    if (!loadObj(key, source, error)) {
        m_failedPaths.push_back(key);
        return nullptr;
    }
    // A file without a single face is a valid OBJ file, but there is nothing to draw.
    if (source.indices.empty()) {
        core::logError("Model has no faces: " + core::pathText(key));
        m_failedPaths.push_back(key);
        return nullptr;
    }

    // Each part of the file becomes a part of the model, with the colour and the texture
    // of its material looked up now, once, instead of in every frame.
    std::vector<ModelPart> parts;
    for (const ObjPart& sourcePart : source.parts) {
        ModelPart part;
        part.material = sourcePart.material;
        part.firstIndex = sourcePart.firstIndex;
        part.indexCount = sourcePart.indexCount;
        part.texture = &m_whiteTexture;

        // loadObj has checked that every named material exists. Faces without a material
        // (an empty name) keep the defaults: white colour, white texture.
        const ObjMaterial* material = findMaterial(source, sourcePart.material);
        if (material != nullptr) {
            part.color = material->diffuseColor;
            part.texturePath = material->diffuseTexture;
        }
        if (!part.texturePath.empty()) {
            // texture() logs a failed load. The part then keeps the white texture and is
            // drawn in its plain colour.
            const gfx::Texture2D* texture = this->texture(part.texturePath);
            if (texture != nullptr) {
                part.texture = texture;
                part.hasOwnTexture = true;
            }
        }
        parts.push_back(std::move(part));
    }

    // The mesh copies the vertices and the indices to the graphics card. The plain data
    // in source is freed when this function returns. emplace_back builds the new element
    // in place and returns a reference to it.
    const LoadedModel& loaded = m_models.emplace_back(LoadedModel{
        .path = key,
        .mesh = gfx::Mesh(source.vertices, source.indices),
        .parts = std::move(parts),
        .vertexCount = source.vertices.size(),
        .triangleCount = source.indices.size() / INDICES_PER_TRIANGLE,
    });
    core::logInfo("Loaded model: " + core::pathText(key));
    return &loaded;
}

const gfx::Texture2D* AssetCache::texture(const std::filesystem::path& path) {
    const std::filesystem::path key = cacheKey(path);

    for (const LoadedTexture& loaded : m_textures) {
        if (loaded.path == key) {
            return &loaded.texture;
        }
    }
    if (hasFailed(key)) {
        return nullptr;
    }

    // loadImage logs its own error.
    Image image;
    std::string error;
    if (!loadImage(key, image, error)) {
        m_failedPaths.push_back(key);
        return nullptr;
    }

    // The constructor logs an error and leaves the texture not valid when the picture
    // has a channel count it does not accept (grey pictures have 1 or 2 channels).
    gfx::Texture2D texture(image.width, image.height, image.channels, image.pixels.data());
    if (!texture.isValid()) {
        core::logError("Texture cannot be used: " + core::pathText(key));
        m_failedPaths.push_back(key);
        return nullptr;
    }
    // A texture loaded later must look like the ones loaded before.
    texture.setFilter(m_filter);
    texture.setAnisotropy(m_anisotropy);

    const LoadedTexture& loaded =
        m_textures.emplace_back(LoadedTexture{.path = key, .texture = std::move(texture)});
    core::logInfo("Loaded texture: " + core::pathText(key));
    return &loaded.texture;
}

void AssetCache::setFilter(gfx::TextureFilter filter) {
    m_filter = filter;
    for (LoadedTexture& loaded : m_textures) {
        loaded.texture.setFilter(filter);
    }
}

void AssetCache::setAnisotropy(float level) {
    // The same clamping as in Texture2D::setAnisotropy, so that anisotropy() reports the
    // level the textures really use. Without the extension the maximum is 1.
    m_anisotropy = std::clamp(level, NO_ANISOTROPY, maxAnisotropy());
    for (LoadedTexture& loaded : m_textures) {
        loaded.texture.setAnisotropy(m_anisotropy);
    }
}

bool AssetCache::hasFailed(const std::filesystem::path& path) const {
    // find returns the end of the list when no element is equal to path.
    return std::ranges::find(m_failedPaths, path) != m_failedPaths.end();
}

} // namespace assets
