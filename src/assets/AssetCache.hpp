// AssetCache: loads every model and every texture once and hands out stable references.
// See docs/modules/assets/asset-cache.md
#pragma once

#include "gfx/Mesh.hpp"
#include "gfx/Texture2D.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

namespace assets {

/// A texture of the cache together with the file it was read from.
struct LoadedTexture {
    /// Path of the image file, normalized. This is the key of the cache.
    std::filesystem::path path;

    /// The picture on the graphics card.
    gfx::Texture2D texture;
};

/// One material of a loaded model: a run of indices of the mesh and what to draw it with.
struct ModelPart {
    /// Name of the material (the usemtl line of the OBJ file). Empty when the faces had
    /// no material.
    std::string material;

    /// The run of indices: gfx::Mesh::draw(firstIndex, indexCount) draws this part.
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;

    /// Diffuse colour of the material (the Kd line). The shader multiplies the texture by
    /// it. White (the default) leaves the texture unchanged.
    glm::vec3 color{1.0F};

    /// Texture to bind for this part. Never null: when the material names no texture, or
    /// the file could not be loaded, it points at the white texture of the cache, and the
    /// part is drawn in its plain colour.
    const gfx::Texture2D* texture = nullptr;

    /// Path of the texture file from the map_Kd line. Empty when the material has none.
    std::filesystem::path texturePath;

    /// False when texture is the white fallback and not the picture of texturePath.
    bool hasOwnTexture = false;
};

/// A model on the graphics card: one mesh, split into parts by material.
struct LoadedModel {
    /// Path of the OBJ file, normalized. This is the key of the cache.
    std::filesystem::path path;

    /// All vertices and indices of the model.
    gfx::Mesh mesh;

    /// The mesh split by material, in file order.
    std::vector<ModelPart> parts;

    /// Numbers for the debug panel. The mesh itself does not keep its vertex count.
    std::size_t vertexCount = 0;
    std::size_t triangleCount = 0;
};

/// Loads models and textures on first request and keeps them until it is destroyed.
/// Asking for the same file again returns the object loaded before, so a texture used by
/// two models (wall_stone.png by the wall and by the pillar) exists once on the card.
///
/// The pointers it returns stay valid for the whole life of the cache: loading more
/// assets never moves the ones already loaded.
///
/// It owns OpenGL objects, so it needs a current OpenGL context for its whole life and
/// must be destroyed before the window. It cannot be copied or moved, because the parts
/// of its models point at its textures.
class AssetCache {
public:
    /// Creates the 1 x 1 white texture that stands in for a missing one.
    AssetCache();

    AssetCache(const AssetCache&) = delete;
    AssetCache& operator=(const AssetCache&) = delete;

    /// The model read from an OBJ file (and the MTL files and textures it names). The
    /// first call for a path loads it, later calls return the same object.
    ///
    /// Returns nullptr when the file cannot be loaded. The error is logged once, by the
    /// first call: the path is remembered as failed and not tried again. A model whose
    /// texture is missing still loads: that part gets the white texture. It does not throw.
    const LoadedModel* model(const std::filesystem::path& path);

    /// The texture read from an image file. The first call for a path loads it, later
    /// calls return the same object. New textures get the filter and the anisotropy
    /// level chosen with setFilter and setAnisotropy.
    ///
    /// Returns nullptr when the file cannot be loaded or is not a picture with 3 or 4
    /// channels. The error is logged once, by the first call. It does not throw.
    const gfx::Texture2D* texture(const std::filesystem::path& path);

    /// A 1 x 1 white texture. A shader that multiplies a colour by a texture draws the
    /// plain colour with it.
    const gfx::Texture2D& whiteTexture() const { return m_whiteTexture; }

    /// Changes the filter of every loaded texture, and of the ones loaded later.
    void setFilter(gfx::TextureFilter filter);

    /// Changes the anisotropy level of every loaded texture, and of the ones loaded
    /// later. The level is clamped to the range from 1 to maxAnisotropy().
    void setAnisotropy(float level);

    /// Filter in use, as set by setFilter. Trilinear at the start.
    gfx::TextureFilter filter() const { return m_filter; }

    /// Anisotropy level in use, after clamping. 1 (off) at the start.
    float anisotropy() const { return m_anisotropy; }

    /// Highest anisotropy level the driver accepts. 1 when it does not offer anisotropic
    /// filtering.
    float maxAnisotropy() const { return m_whiteTexture.maxAnisotropy(); }

    /// Everything loaded so far, in the order of loading, for the debug panel.
    const std::deque<LoadedModel>& models() const { return m_models; }
    const std::deque<LoadedTexture>& textures() const { return m_textures; }

    /// Paths of the models and textures that could not be loaded, for the debug panel.
    const std::vector<std::filesystem::path>& failedPaths() const { return m_failedPaths; }

private:
    /// True when the path is in m_failedPaths.
    bool hasFailed(const std::filesystem::path& path) const;

    // The stand-in for a missing texture. Declared first, because the parts of the models
    // below may point at it.
    gfx::Texture2D m_whiteTexture;

    // std::deque and not std::vector: adding an element at the end of a deque never moves
    // the elements that are already in it, so the pointers handed out stay valid. A vector
    // moves all of its elements to a new block of memory when it runs out of room.
    //
    // The textures are declared before the models, so they are destroyed after them
    // (members are destroyed bottom to top): no part ever points at a deleted texture.
    std::deque<LoadedTexture> m_textures;
    std::deque<LoadedModel> m_models;
    std::vector<std::filesystem::path> m_failedPaths;

    gfx::TextureFilter m_filter = gfx::TextureFilter::Trilinear;
    float m_anisotropy = 1.0F;
};

} // namespace assets
