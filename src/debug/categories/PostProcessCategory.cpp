// "Post process" category of the debug window: exposure and tone mapping of the composite
// pass, the bloom, the fog, the vignette, and previews of the scene framebuffer and of
// the bloom targets.
#include "debug/categories/PostProcessCategory.hpp"

#include "debug/DebugContext.hpp"
#include "debug/Pictures.hpp"
#include "debug/Widgets.hpp"
#include "game/PostProcess.hpp"
#include "gfx/Framebuffer.hpp"

#include <imgui.h>

namespace debug {

namespace {

// The entries of the list, in the order of the enum game::ToneMapping: the number of the
// chosen entry is the value of the enum. ImGui wants the entries in one string, each
// ended by a zero character.
constexpr const char* TONE_MAPPING_ITEMS = "None (clamp)\0Reinhard\0ACES (fitted)\0";

// Range of the exposure slider. 1 leaves the scene as it is drawn. The slider is
// logarithmic, so halving and doubling the light are steps of the same length.
constexpr float MIN_EXPOSURE = 0.1F;
constexpr float MAX_EXPOSURE = 8.0F;

// Range of the bloom threshold slider, as a brightness in the HDR buffer. At 0 the
// whole picture takes part in the bloom. The upper end is above everything the scene
// draws with its default lights, so there the bloom finds nothing.
constexpr float MIN_BLOOM_THRESHOLD = 0.0F;
constexpr float MAX_BLOOM_THRESHOLD = 4.0F;

// Range of the bloom intensity slider. 0 adds no glow at all.
constexpr float MIN_BLOOM_INTENSITY = 0.0F;
constexpr float MAX_BLOOM_INTENSITY = 2.0F;

// Range of the slider of the depth preview, in metres: the distance shown as white.
constexpr float MIN_DEPTH_RANGE = 2.0F;
constexpr float MAX_DEPTH_RANGE = 100.0F;

// Range of the fog density slider, per metre. 0 is no fog. At the upper end half of
// a surface is gone after 1.4 m: the maze can hardly be seen.
constexpr float MIN_FOG_DENSITY = 0.0F;
constexpr float MAX_FOG_DENSITY = 0.5F;

// Range of the slider of the height up to which the fog has its full density, in metres
// of world height: from below the lowest ground to above the tops of the walls.
constexpr float MIN_FOG_BASE_HEIGHT = -2.0F;
constexpr float MAX_FOG_BASE_HEIGHT = 6.0F;

// Range of the slider of the height falloff of the fog, per metre. 0 gives the same
// density at every height. At the upper end the fog is a layer about a metre thick.
constexpr float MIN_FOG_HEIGHT_FALLOFF = 0.0F;
constexpr float MAX_FOG_HEIGHT_FALLOFF = 3.0F;

// Range of the vignette strength slider: the share of the light the corners lose.
constexpr float MIN_VIGNETTE_STRENGTH = 0.0F;
constexpr float MAX_VIGNETTE_STRENGTH = 1.0F;

// Range of the vignette radius slider, in texture coordinates from the middle of the
// screen. The upper end stays below the distance to a corner
// (game::VIGNETTE_CORNER_DISTANCE): the darkening needs some room to fade in.
constexpr float MIN_VIGNETTE_RADIUS = 0.0F;
constexpr float MAX_VIGNETTE_RADIUS = 0.65F;

// The two numbers of the composite pass (post/composite.frag).
void drawToneMapping(Page& page, game::PostProcessSettings& settings) {
    page.beginCard("Tone mapping");
    page.slider("Exposure", &settings.exposure, MIN_EXPOSURE, MAX_EXPOSURE, "%.2f",
                "The colours of the scene are multiplied by this number before tone "
                "mapping. 1 changes nothing. Logarithmic: halving and doubling the light "
                "are steps of the same length.",
                true);
    int toneMappingIndex = static_cast<int>(settings.toneMapping);
    if (page.combo("Tone mapping", &toneMappingIndex, TONE_MAPPING_ITEMS,
                   "How colours brighter than 1 are brought into the range of the screen. "
                   "The debug views (normals, UVs) are shown without exposure, tone "
                   "mapping, bloom, fog and vignette.")) {
        settings.toneMapping = static_cast<game::ToneMapping>(toneMappingIndex);
    }
    page.endCard();
}

// The switch and the three numbers of the bloom (game::BloomSettings).
void drawBloom(Page& page, game::BloomSettings& bloom) {
    page.beginCard("Bloom");
    page.toggle("Bloom", &bloom.enabled,
                "Bright parts of the scene glow: they are copied into a smaller picture, "
                "blurred and added back. The debug views (normals, UVs) are shown "
                "without it.");
    page.sliderInt("Blur iterations", &bloom.blurIterations, game::MIN_BLOOM_BLUR_ITERATIONS,
                   game::MAX_BLOOM_BLUR_ITERATIONS, "%d",
                   "How many times the Gaussian blur of the bloom runs (a horizontal and "
                   "a vertical pass each time). More makes the glow wider.");
    page.slider("Threshold", &bloom.threshold, MIN_BLOOM_THRESHOLD, MAX_BLOOM_THRESHOLD, "%.2f",
                "Bloom: only light brighter than this glows. 1 is the white of the screen "
                "before exposure. The bright pass preview shows what is left.");
    page.slider("Intensity", &bloom.intensity, MIN_BLOOM_INTENSITY, MAX_BLOOM_INTENSITY, "%.2f",
                "Bloom: the blurred glow is multiplied by this number before it is added "
                "to the scene.");
    page.endCard();
}

// The switch, the three numbers and the colour of the fog (game::FogSettings).
void drawFog(Page& page, game::FogSettings& fog) {
    page.beginCard("Fog");
    page.toggle("Fog", &fog.enabled,
                "Far and low surfaces fade into the fog colour. Computed in the composite "
                "pass from the depth of the scene. The debug views (normals, UVs) are "
                "shown without it.");
    page.slider("Density", &fog.density, MIN_FOG_DENSITY, MAX_FOG_DENSITY, "%.3f /m",
                "Fog: amount = 1 - exp(-density * height factor * distance). At 0.1, half "
                "of a surface on the ground is gone after 6.9 m.");
    page.slider("Base height", &fog.baseHeight, MIN_FOG_BASE_HEIGHT, MAX_FOG_BASE_HEIGHT, "%.2f m",
                "Fog: up to this world height the fog has its full density. The ground of "
                "the maze reaches about 0.6 m at height scale 1. This number does not "
                "follow the height scale.");
    page.slider("Height falloff", &fog.heightFalloff, MIN_FOG_HEIGHT_FALLOFF,
                MAX_FOG_HEIGHT_FALLOFF, "%.2f /m",
                "Fog: how fast it thins out above the base height: height factor = "
                "exp(-falloff * metres above the base). 0 gives the same fog at every "
                "height, the sky included.");
    // &fog.color.x is the address of the first of the three floats of the vector, which
    // lie next to each other: the array of three floats a colour row asks for.
    page.color("Fog colour", &fog.color.x,
               "The colour surfaces fade into, as an sRGB value. It is mixed in before "
               "exposure and tone mapping: with Reinhard or ACES the fog on the screen is "
               "darker than this swatch, with None (clamp) at exposure 1 it matches.");
    page.endCard();
}

// The switch and the two numbers of the vignette (game::VignetteSettings).
void drawVignette(Page& page, game::VignetteSettings& vignette) {
    page.beginCard("Vignette");
    page.toggle("Vignette", &vignette.enabled,
                "The corners of the finished picture are darkened, after tone mapping. The "
                "debug views (normals, UVs) are shown without it.");
    page.slider("Strength", &vignette.strength, MIN_VIGNETTE_STRENGTH, MAX_VIGNETTE_STRENGTH,
                "%.2f",
                "Vignette: the share of the light the corners lose. 0 changes nothing, "
                "1 makes them black.");
    page.slider("Radius", &vignette.radius, MIN_VIGNETTE_RADIUS, MAX_VIGNETTE_RADIUS, "%.2f",
                "Vignette: the distance from the middle of the screen at which the "
                "darkening starts. 0.5 is the middle of an edge, 0.71 a corner. Not "
                "corrected for the shape of the window.");
    page.endCard();
}

// The framebuffer the scene is drawn into and the smaller targets of the bloom: their
// sizes and formats, and four pictures of what is in them.
void drawPreviews(Page& page, const DebugContext& context) {
    game::PostProcessSettings& settings = context.postProcessSettings;
    const game::PostProcess& postProcess = context.postProcess;
    const gfx::Framebuffer& scene = postProcess.sceneTarget();
    const gfx::Framebuffer& bloom = postProcess.bloomTarget();
    const bool bloomDrawn = postProcess.bloomDrawn();

    page.beginCard("Previews");
    page.slider("Depth range", &settings.depthPreviewRange, MIN_DEPTH_RANGE, MAX_DEPTH_RANGE,
                "%.0f m",
                "The depth preview shows the distance from the camera: black at 0 m, "
                "white at this distance and beyond.");
    page.stat("Scene framebuffer", "%d x %d px, %s + %s", scene.width(), scene.height(),
              gfx::colorFormatName(scene.colorFormat()), gfx::depthFormatName(scene.depthFormat()));
    if (bloomDrawn) {
        page.stat("Bloom targets (3)", "%d x %d px, %s", bloom.width(), bloom.height(),
                  gfx::colorFormatName(bloom.colorFormat()));
    } else {
        page.stat("Bloom targets", "not drawn (bloom off or a debug view)");
    }

    if (page.beginBlock("previews pictures HDR colour depth bright pass bloom")) {
        // The pictures are asked for only while this block is drawn: the game reads the
        // flag in its next frame, and DebugUI::draw clears it before every frame.
        settings.previews = true;

        // Two pictures side by side, sharing the width of the card: the two attachments
        // of the scene framebuffer first, then the two steps of the bloom.
        const float spacing = ImGui::GetStyle().ItemSpacing.x;
        const float width = (ImGui::GetContentRegionAvail().x - spacing) / 2.0F;
        drawFramebufferPicture("HDR colour", "The colour attachment of the scene, cut off at 1.",
                               postProcess.preview(game::AttachmentPreview::Color), width, true);
        ImGui::SameLine();
        drawFramebufferPicture("Depth", "The depth attachment of the scene, as a distance.",
                               postProcess.preview(game::AttachmentPreview::Depth), width, true);
        drawFramebufferPicture("Bright pass", "What the scene has above the bloom threshold.",
                               postProcess.brightPassPreview(), width, bloomDrawn);
        ImGui::SameLine();
        drawFramebufferPicture("Bloom", "The bright pass after the blur, before the intensity.",
                               postProcess.bloomPreview(), width, bloomDrawn);
    }
    page.endCard();
}

} // namespace

void drawPostProcessCategory(Page& page, const DebugContext& context) {
    game::PostProcessSettings& settings = context.postProcessSettings;
    page.setPlace("Post process");
    page.beginColumns();
    drawToneMapping(page, settings);
    drawBloom(page, settings.bloom);
    drawFog(page, settings.fog);
    page.nextColumn();
    drawVignette(page, settings.vignette);
    drawPreviews(page, context);
    page.endColumns();
}

} // namespace debug
