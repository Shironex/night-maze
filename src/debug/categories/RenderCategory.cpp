// "Render" category of the debug window: lighting mode, sky, clear colour, the view of
// the textured shader and the texture filtering.
#include "debug/categories/RenderCategory.hpp"

#include "assets/AssetCache.hpp"
#include "debug/DebugContext.hpp"
#include "debug/Widgets.hpp"
#include "game/Lighting.hpp"
#include "game/MazeRenderer.hpp"
#include "game/Skybox.hpp"
#include "gfx/Texture2D.hpp"

namespace debug {

namespace {

// The entries of the three lists, in the order of the enums game::LightingMode,
// game::ViewMode and gfx::TextureFilter: the number of the chosen entry is the value of
// the enum. ImGui wants the entries in one string, each ended by a zero character.
constexpr const char* LIGHTING_MODE_ITEMS = "Unlit\0Gouraud\0Phong\0Blinn-Phong\0";
constexpr const char* VIEW_MODE_ITEMS = "Textured\0Normals as colour\0UVs as colour\0";
constexpr const char* FILTER_ITEMS = "Nearest\0Bilinear\0Trilinear\0";

// Range of the slider of the sky brightness. 1 leaves the sky pictures as they are, 0 is
// a black sky. The pictures are dark, so the range goes well above 1.
constexpr float MIN_SKY_BRIGHTNESS = 0.0F;
constexpr float MAX_SKY_BRIGHTNESS = 6.0F;

// Anisotropy level 1 means "off", and a driver without the extension reports 1 as its
// maximum.
constexpr float NO_ANISOTROPY = 1.0F;

// How the maze is shaded, the sky and the background behind it.
void drawScene(Page& page, const DebugContext& context) {
    page.beginCard("Scene");

    // A list works on the number of the chosen entry, so the enum is turned into a
    // number and back. combo returns true in the frame in which the user picked
    // another entry.
    int lightingModeIndex = static_cast<int>(context.lighting.mode);
    if (page.combo("Lighting", &lightingModeIndex, LIGHTING_MODE_ITEMS,
                   "How the maze is shaded. Unlit: the textures as they are. Gouraud: the "
                   "light is computed for every vertex. Phong and Blinn-Phong: for every "
                   "pixel, with two formulas for the highlight.")) {
        context.lighting.mode = static_cast<game::LightingMode>(lightingModeIndex);
    }

    // The sky. Switched off, the clear colour below is the background again.
    page.toggle("Skybox", &context.skybox.enabled,
                "The night sky (a cube map). The painted moon stands where the default "
                "moon light comes from and does not follow the Moon sliders of the Light "
                "category.");
    page.slider("Sky brightness", &context.skybox.brightness, MIN_SKY_BRIGHTNESS,
                MAX_SKY_BRIGHTNESS, "%.3f",
                "How bright the sky is drawn. 1 leaves its pictures as they are, 0 is "
                "a black sky.");
    // data() is the address of the three floats of the array.
    page.color("Clear colour", context.clearColor.data(),
               "The background where the sky is not drawn.");

    page.endCard();
}

// What the textured shader shows and how every texture is filtered.
void drawTextures(Page& page, const DebugContext& context) {
    page.beginCard("Textures and normals");

    int viewModeIndex = static_cast<int>(context.viewMode);
    if (page.combo("View mode", &viewModeIndex, VIEW_MODE_ITEMS,
                   "What the scene shows: its textures, or one of two debug views: the "
                   "normal used for shading as a colour, or the texture coordinate as "
                   "a colour.")) {
        context.viewMode = static_cast<game::ViewMode>(viewModeIndex);
    }

    // The game reads the switch in every frame, so the scene changes at once: the same
    // walls with and without their relief. It stands next to the view mode, because
    // the two together decide which normals are shown.
    page.toggle("Normal mapping", &context.lighting.normalMapping,
                "Shows under Phong and Blinn-Phong lighting and in the view \"Normals as "
                "colour\". Gouraud lights per vertex and cannot use a normal map.");

    assets::AssetCache& assets = context.assets;
    int filterIndex = static_cast<int>(assets.filter());
    if (page.combo("Filter", &filterIndex, FILTER_ITEMS,
                   "Applies to all textures. Its effect shows in the scene, not in the "
                   "texture previews of the Diagnostics category.")) {
        assets.setFilter(static_cast<gfx::TextureFilter>(filterIndex));
    }

    // Without the extension there is nothing to choose: the slider is shown greyed
    // out with a note, so it is clear why it does nothing.
    const float maxAnisotropy = assets.maxAnisotropy();
    const bool anisotropySupported = maxAnisotropy > NO_ANISOTROPY;
    float anisotropy = assets.anisotropy();
    page.disableNextRow(!anisotropySupported);
    if (page.slider("Anisotropy", &anisotropy, NO_ANISOTROPY, maxAnisotropy, "%.0fx",
                    "Anisotropic filtering keeps a texture sharp on a surface seen at "
                    "a flat angle. 1 is off. The upper end is what the graphics driver "
                    "offers. It applies to all textures.")) {
        assets.setAnisotropy(anisotropy);
    }
    if (!anisotropySupported) {
        page.note("Anisotropic filtering is not offered by this graphics driver.");
    }

    page.endCard();
}

} // namespace

void drawRenderCategory(Page& page, const DebugContext& context) {
    page.setPlace("Render");
    page.beginColumns();
    drawScene(page, context);
    page.nextColumn();
    drawTextures(page, context);
    page.endColumns();
}

} // namespace debug
