// "Assets" debug panel: loaded models and textures.
// See docs/modules/assets/asset-cache.md
#include "debug/panels/AssetsPanel.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "debug/PanelLayout.hpp"
#include "debug/RawTextureSampler.hpp"
#include "debug/Theme.hpp"
#include "gfx/Texture2D.hpp"

#include <imgui.h>

#include <cstdint>
#include <filesystem>
#include <string>

namespace debug {

namespace {

// A triangle is three indices.
constexpr std::uint32_t INDICES_PER_TRIANGLE = 3;

// Side of the square preview of a texture, in pixels.
constexpr float PREVIEW_SIZE = 128.0F;

// One line with the file name. The full path appears as a tooltip when the mouse rests
// on the line. ImGui expects UTF-8, which core::pathText returns.
void drawFileName(const std::filesystem::path& path) {
    const std::string fileName = core::pathText(path.filename());
    const std::string fullPath = core::pathText(path);
    ImGui::TextUnformatted(fileName.c_str());
    ImGui::SetItemTooltip("%s", fullPath.c_str());
}

// The list of loaded models: file, sizes and the parts with their materials, textures
// and normal maps.
void drawModels(const assets::AssetCache& assets) {
    ImGui::SeparatorText("Models");
    for (const assets::LoadedModel& model : assets.models()) {
        drawFileName(model.path);
        ImGui::Text("  %d vertices, %d triangles", static_cast<int>(model.vertexCount),
                    static_cast<int>(model.triangleCount));
        for (const assets::ModelPart& part : model.parts) {
            // A part without its own texture is drawn with the white one, in its colour.
            const std::string textureName = part.hasOwnTexture
                                                ? core::pathText(part.texturePath.filename())
                                                : std::string("no texture (white)");
            ImGui::Text("  part '%s': %d triangles, %s", part.material.c_str(),
                        static_cast<int>(part.indexCount / INDICES_PER_TRIANGLE),
                        textureName.c_str());
            // A part without its own normal map is shaded with the flat one: with the
            // normals of its mesh.
            const std::string normalMapName = part.hasOwnNormalMap
                                                  ? core::pathText(part.normalMapPath.filename())
                                                  : std::string("none (flat)");
            ImGui::Text("    normal map: %s", normalMapName.c_str());
        }
    }
}

// The list of loaded textures: file, size, colour space and a small picture. The normal
// maps are in the same list: for the cache they are textures like the others. Their
// preview is the picture as it is stored, mostly light blue, because most texels hold
// a direction close to (0, 0, 1), which is the colour (128, 128, 255).
void drawTextures(const assets::AssetCache& assets, const RawTextureSampler& rawSampler) {
    ImGui::SeparatorText("Textures");
    for (const assets::LoadedTexture& loaded : assets.textures()) {
        drawFileName(loaded.path);
        // sRGB: a colour picture, decoded to linear values when a shader reads it.
        // Linear: data that is read as it is stored (a normal map).
        const bool isSrgb = loaded.texture.colorSpace() == gfx::ColorSpace::Srgb;
        ImGui::Text("  %d x %d px, %s", loaded.texture.width(), loaded.texture.height(),
                    isSrgb ? "sRGB" : "linear");

        // ImGui identifies a texture by a number it hands to its renderer: for the OpenGL
        // backend that is the id of the texture object. The cast only widens the number
        // to the integer type ImGui uses.
        //
        // uv0 is the texture coordinate of the top left corner of the picture and uv1 of
        // the bottom right one. In OpenGL v = 0 is the BOTTOM row, so the top left corner
        // is (0, 1) and the bottom right one (1, 0). With the defaults, (0, 0) and
        // (1, 1), the preview would be upside down.
        //
        // The preview is drawn by ImGui with its own sampler, always with a linear
        // filter, so it does not react to the filter chosen in the Render category.
        //
        // An sRGB texture read by ImGui would give linear values, and ImGui writes what
        // it reads straight into the window: the picture would be too dark. Between
        // begin and end it is read without the decoding, so the preview shows the
        // bytes of the file.
        const auto textureId = static_cast<ImTextureID>(loaded.texture.id());
        if (isSrgb) {
            rawSampler.begin();
        }
        ImGui::Image(textureId, {PREVIEW_SIZE, PREVIEW_SIZE}, {0.0F, 1.0F}, {1.0F, 0.0F});
        if (isSrgb) {
            rawSampler.end();
        }
    }
}

// The files that could not be loaded, if there are any.
void drawFailures(const assets::AssetCache& assets) {
    if (assets.failedPaths().empty()) {
        return;
    }
    ImGui::SeparatorText("Failed to load");
    // The red of the text is a colour of the theme (Theme.hpp), shared with the Shaders
    // panel.
    ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
    for (const std::filesystem::path& path : assets.failedPaths()) {
        drawFileName(path);
    }
    ImGui::PopStyleColor();
}

} // namespace

void drawAssetsPanel(const assets::AssetCache& assets, const RawTextureSampler& rawSampler) {
    // First run only: the right edge of the window, below the Maze panel (the constant
    // is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(ASSETS_PLACEMENT);
    if (ImGui::Begin("Assets")) {
        drawModels(assets);
        drawTextures(assets, rawSampler);
        drawFailures(assets);
    }
    ImGui::End();
}

} // namespace debug
