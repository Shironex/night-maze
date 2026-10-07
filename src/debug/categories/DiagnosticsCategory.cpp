// "Diagnostics" category of the debug window: frame statistics and driver info, the
// sound device with the last sound cue, the shader programs with a reload button and
// their errors, the collision boxes and the picking ray, and the loaded models and
// textures.
// See docs/modules/gfx/shader-hot-reload.md, docs/modules/scene/collision.md and
// docs/modules/assets/asset-cache.md
#include "debug/categories/DiagnosticsCategory.hpp"

#include "assets/AssetCache.hpp"
#include "audio/AudioEngine.hpp"
#include "core/Paths.hpp"
#include "core/Time.hpp"
#include "core/Window.hpp"
#include "debug/DebugContext.hpp"
#include "debug/RawTextureSampler.hpp"
#include "debug/Theme.hpp"
#include "debug/Widgets.hpp"
#include "game/Interactables.hpp"
#include "game/Interaction.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "gfx/Shader.hpp"
#include "gfx/Texture2D.hpp"
#include "scene/Collider.hpp"

#include <imgui.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>

namespace debug {

namespace {

// How many shader programs the game has.
constexpr int SHADER_COUNT = 14;

// A triangle is three indices.
constexpr std::uint32_t INDICES_PER_TRIANGLE = 3;

// Side of the square preview of a texture, in pixels at 100 % display scaling.
constexpr float TEXTURE_PREVIEW_SIZE = 128.0F;

// ---- Frame and shaders -----------------------------------------------------------------

// The frame clock, the sizes of the window and what the graphics driver says it is.
void drawFrame(Page& page, const DebugContext& context) {
    const core::Size framebuffer = context.window.framebufferSize();
    const core::Size windowSize = context.window.windowSize();

    page.beginCard("Frame");
    page.stat("FPS", "%.1f", context.time.fps());
    page.stat("Frame time", "%.2f ms", context.time.frameTimeMs());
    page.stat("Framebuffer", "%d x %d px", framebuffer.width, framebuffer.height);
    page.stat("Window", "%d x %d", windowSize.width, windowSize.height);
    page.stat("OpenGL", "%s", context.window.glVersion().c_str());
    page.stat("GPU", "%s", context.window.glRenderer().c_str());
    page.endCard();
}

// The sound device, the last sound cue and the master volume, all read only.
void drawAudio(Page& page, const DebugContext& context) {
    page.beginCard("Audio");
    // The device with its format and the number of loaded sounds, or why there is no
    // sound (audio::AudioEngine::status).
    page.stat("Device", "%s", context.audio.status().c_str());
    page.stat("Last cue", "%s", context.lastCueName);
    page.stat("Cues played", "%d", context.cuesPlayed);
    // The volume is a setting of the player (game/Settings.hpp): it is changed on the
    // settings screen, which also saves it, and only shown here. The second number is
    // what the engine was given for it (game::masterVolumeGain).
    page.stat("Master volume", "%.0f of 100 (gain %.2f), set on the settings screen",
              context.masterVolume, context.audio.masterVolume());
    page.endCard();
}

// One program: its name and how its last load went, under it its files, and under
// them the error message of a failed load.
void drawShaderStatus(const gfx::Shader& shader) {
    // The name is the name of the fragment shader without its ending ("lit" for
    // lit.frag): every program has a fragment shader of its own. The line below shows
    // the file names, joined by " + " in the order the stages run: the vertex shader,
    // the geometry shader of a program that has one, the fragment shader. The full
    // paths, one per line, are in the tooltip. ImGui expects UTF-8, which
    // core::pathText returns.
    const std::string name = core::pathText(shader.fragmentPath().stem());
    std::string files = core::pathText(shader.vertexPath().filename());
    std::string fullPaths = core::pathText(shader.vertexPath());
    if (shader.hasGeometryStage()) {
        files += " + " + core::pathText(shader.geometryPath().filename());
        fullPaths += "\n" + core::pathText(shader.geometryPath());
    }
    files += " + " + core::pathText(shader.fragmentPath().filename());
    fullPaths += "\n" + core::pathText(shader.fragmentPath());

    const bool failed = !shader.lastError().empty();
    // A group makes the lines of one program one widget, so one tooltip covers them.
    ImGui::BeginGroup();
    ImGui::TextUnformatted(name.c_str());
    // The result of the last load at the right edge of the card: the second colour for
    // a program that is fine, the red of an error (Theme.hpp) for one that is not.
    const char* status = failed ? "FAILED" : "OK";
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x -
                         ImGui::CalcTextSize(status).x);
    ImGui::TextColored(failed ? ERROR_TEXT_COLOR : SECONDARY_COLOR, "%s", status);

    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_FAINT_COLOR);
    // "%s" and the text as an argument: the text itself is never read as a format.
    ImGui::TextWrapped("%s", files.c_str());
    ImGui::PopStyleColor();

    if (failed) {
        // A failed load. Valid means that there is still a linked program to draw with:
        // the one from before the failed reload. Without one (the very first load
        // failed) nothing is drawn with this program. The message contains text
        // written by the driver. For an error inside an included file it names that
        // file (gfx::nameSourceFiles).
        ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
        ImGui::TextWrapped("%s", shader.isValid() ? "The previous program stays in use."
                                                  : "There is no program to draw with.");
        ImGui::TextWrapped("%s", shader.lastError().c_str());
        ImGui::PopStyleColor();
    }
    ImGui::EndGroup();
    tooltipCard(name.c_str(), fullPaths.c_str());
}

// The reload button and the list of all programs.
void drawShaders(Page& page, const DebugContext& context) {
    // A list of all programs, so that a new program is one more entry here. The array
    // holds pointers, because a reference cannot be an element of an array.
    const std::array<gfx::Shader*, SHADER_COUNT> shaders = {
        &context.texturedShader,       &context.colorShader,       &context.litShader,
        &context.gouraudShader,        &context.skyboxShader,      &context.grassShader,
        &context.compositeShader,      &context.previewShader,     &context.brightPassShader,
        &context.blurShader,           &context.shadowDepthShader, &context.minimapShader,
        &context.minimapOverlayShader, &context.reflectShader};

    page.beginCard("Shaders");
    // One button reloads every program: after editing a file there is no need to know
    // which program it belongs to, and a file that several programs include
    // (common/lighting.glsl) is read again by each of them. The results of reload()
    // are not needed here: the lines below read them from lastError(). A program whose
    // reload fails keeps working with its previous version, and the others are
    // reloaded all the same.
    if (page.buttons("Shaders", "Reload shaders", nullptr,
                     "Reads the files of all 14 shader programs again and builds them. "
                     "A program that fails keeps its previous version.") == 1) {
        for (gfx::Shader* shader : shaders) {
            shader->reload();
        }
    }

    if (page.beginBlock("shader programs list status errors files vert frag geom")) {
        for (const gfx::Shader* shader : shaders) {
            drawShaderStatus(*shader);
        }
    }
    page.endCard();
}

void drawFrameAndShadersTab(Page& page, const DebugContext& context) {
    page.setPlace("Diagnostics / Frame and shaders");
    page.beginColumns();
    drawFrame(page, context);
    drawAudio(page, context);
    page.nextColumn();
    drawShaders(page, context);
    page.endColumns();
}

// ---- Collision and picking -------------------------------------------------------------

// What the picking ray hit, in words.
const char* kindName(game::InteractableKind kind) {
    switch (kind) {
    case game::InteractableKind::Lever:
        return "lever";
    case game::InteractableKind::Note:
        return "note";
    case game::InteractableKind::None:
        break;
    }
    return "nothing";
}

// What the interaction key does, in words.
const char* actionName(game::Interaction action) {
    switch (action) {
    case game::Interaction::PullLever:
        return "pull the lever";
    case game::Interaction::ReadNote:
        return "read the note";
    case game::Interaction::CloseNote:
        return "close the note card";
    case game::Interaction::None:
        break;
    }
    return "nothing";
}

// The switches that draw the shapes the game computes with, as lines in the scene.
void drawDebugDrawing(Page& page, const DebugContext& context) {
    game::PickDebugSettings& pickDebug = context.pickDebug;
    page.beginCard("Debug drawing");
    page.toggle("Draw collision shapes", &context.drawColliders,
                "Draws the collision boxes and the pickup spheres as lines in the scene. "
                "The line below names their colours.");
    page.note("Yellow: walls, pillars. Green: player. Orange: gate. Cyan: crystal pickup. "
              "Magenta: exit zone.");
    page.toggle("Draw pick boxes and ray", &pickDebug.drawShapes,
                "Draws the boxes of the levers and of the notes and the picking ray as "
                "lines in the scene. The line below names their colours.");
    // The ray leaves the eye, so from the eye it is a point. Frozen, it stays where it
    // was and can be looked at from the side.
    page.toggle("Freeze the drawn ray", &pickDebug.freezeRay,
                "The ray leaves the eye, so from the eye it is only a point. Frozen, it "
                "stays where it was and can be looked at from the side.");
    page.note("Red: lever boxes. White: note boxes. Green: the ray and the box it hit. "
              "Grey: a ray that hit nothing.");
    page.endCard();
}

// How many boxes and spheres the collision works with, and the box of the player.
void drawCollisionBoxes(Page& page, const DebugContext& context) {
    const game::MazeWorld& world = context.mazeWorld;
    const game::Round& round = context.round;

    page.beginCard("Collision boxes");
    // world.colliders holds the box of every wall first and the box of every pillar
    // after them, so the two counts are the sizes of the lists they were made from.
    // The gate is one more box while it is closed, and every wall that a lever has
    // opened is one box less (game::roundObstacles).
    const int gateBoxes = game::gateBlocks(world, round) ? 1 : 0;
    const int openedWalls = game::pulledLeverCount(round);
    page.stat("Boxes", "%d walls (%d opened by levers), %d pillars, %d gate",
              static_cast<int>(world.walls.size()) - openedWalls, openedWalls,
              static_cast<int>(world.pillars.size()), gateBoxes);
    // One pickup sphere around every crystal that is not collected yet.
    page.stat("All boxes", "%d, pickup spheres: %d",
              static_cast<int>(world.colliders.size()) - openedWalls + gateBoxes,
              static_cast<int>(round.crystals.size()) - round.collectedCount);
    page.stat("Wall box", "%.2f m thick (the visible wall: %.2f m)", game::WALL_COLLISION_THICKNESS,
              game::WALL_VISUAL_THICKNESS);

    // The box is computed from the position of the player in every frame, exactly as
    // the movement code does it.
    const scene::Aabb box = context.player.box();
    page.stat("Player box min", "%.2f, %.2f, %.2f", box.min.x, box.min.y, box.min.z);
    page.stat("Player box max", "%.2f, %.2f, %.2f", box.max.x, box.max.y, box.max.z);
    page.endCard();
}

// The picking (object selection by ray casting): the result of the ray of the last
// frame: where it starts, where it points, what it hit and what the interaction key
// does.
void drawLastRay(Page& page, const game::PickState& pick) {
    page.beginCard("Last picking ray");
    if (!pick.hasRay) {
        page.stat("Ray", "none (cursor over the debug window or outside)");
        page.stat("Key E", "%s", actionName(pick.action));
        page.endCard();
        return;
    }
    page.stat("Ray through", "%s", pick.centered ? "the middle of the picture" : "the cursor");
    page.stat("Origin", "%.2f, %.2f, %.2f", pick.ray.origin.x, pick.ray.origin.y,
              pick.ray.origin.z);
    page.stat("Direction", "%.3f, %.3f, %.3f", pick.ray.direction.x, pick.ray.direction.y,
              pick.ray.direction.z);
    if (pick.picked.kind == game::InteractableKind::None) {
        page.stat("Hit", "nothing within %.1f m", game::INTERACTION_REACH);
    } else {
        page.stat("Hit", "%s %d at %.2f m", kindName(pick.picked.kind),
                  static_cast<int>(pick.picked.index), pick.picked.distance);
    }
    page.stat("Key E", "%s", actionName(pick.action));
    page.endCard();
}

void drawCollisionAndPickingTab(Page& page, const DebugContext& context) {
    page.setPlace("Diagnostics / Collision and picking");
    page.beginColumns();
    drawDebugDrawing(page, context);
    page.nextColumn();
    drawCollisionBoxes(page, context);
    drawLastRay(page, context.pick);
    page.endColumns();
}

// ---- Assets ----------------------------------------------------------------------------

// One line with the file name. The full path appears as a tooltip when the mouse rests
// on the line.
void drawFileName(const std::filesystem::path& path) {
    const std::string fileName = core::pathText(path.filename());
    const std::string fullPath = core::pathText(path);
    ImGui::TextUnformatted(fileName.c_str());
    tooltipCard(fileName.c_str(), fullPath.c_str());
}

// The list of loaded models: file, sizes and the parts with their materials, textures
// and normal maps.
void drawModels(Page& page, const assets::AssetCache& assets) {
    page.beginCard("Models");
    if (page.beginBlock("models list obj vertices triangles parts materials")) {
        for (const assets::LoadedModel& model : assets.models()) {
            drawFileName(model.path);
            // The facts of a model are quieter than its name and wrap at the edge of
            // the card.
            ImGui::PushStyleColor(ImGuiCol_Text, TEXT_DIM_COLOR);
            ImGui::TextWrapped("%d vertices, %d triangles", static_cast<int>(model.vertexCount),
                               static_cast<int>(model.triangleCount));
            for (const assets::ModelPart& part : model.parts) {
                // A part without its own texture is drawn with the white one, in its
                // colour.
                const std::string textureName = part.hasOwnTexture
                                                    ? core::pathText(part.texturePath.filename())
                                                    : std::string("no texture (white)");
                ImGui::TextWrapped("part '%s': %d triangles, %s", part.material.c_str(),
                                   static_cast<int>(part.indexCount / INDICES_PER_TRIANGLE),
                                   textureName.c_str());
                // A part without its own normal map is shaded with the flat one: with
                // the normals of its mesh.
                const std::string normalMapName =
                    part.hasOwnNormalMap ? core::pathText(part.normalMapPath.filename())
                                         : std::string("none (flat)");
                ImGui::TextWrapped("normal map: %s", normalMapName.c_str());
            }
            ImGui::PopStyleColor();
        }
    }
    page.endCard();
}

// The list of loaded textures: file, size, colour space and a small picture. The normal
// maps are in the same list: for the cache they are textures like the others. Their
// preview is the picture as it is stored, mostly light blue, because most texels hold
// a direction close to (0, 0, 1), which is the colour (128, 128, 255).
void drawTextures(Page& page, const assets::AssetCache& assets,
                  const RawTextureSampler& rawSampler) {
    page.beginCard("Textures");
    if (page.beginBlock("textures list png previews pictures srgb linear normal maps")) {
        const float previewSize = TEXTURE_PREVIEW_SIZE * displayScale();
        for (const assets::LoadedTexture& loaded : assets.textures()) {
            drawFileName(loaded.path);
            // sRGB: a colour picture, decoded to linear values when a shader reads it.
            // Linear: data that is read as it is stored (a normal map).
            const bool isSrgb = loaded.texture.colorSpace() == gfx::ColorSpace::Srgb;
            ImGui::TextDisabled("%d x %d px, %s", loaded.texture.width(), loaded.texture.height(),
                                isSrgb ? "sRGB" : "linear");

            // ImGui identifies a texture by a number it hands to its renderer: for the
            // OpenGL backend that is the id of the texture object. The cast only widens
            // the number to the integer type ImGui uses.
            //
            // uv0 is the texture coordinate of the top left corner of the picture and
            // uv1 of the bottom right one. In OpenGL v = 0 is the BOTTOM row, so the
            // top left corner is (0, 1) and the bottom right one (1, 0). With the
            // defaults, (0, 0) and (1, 1), the preview would be upside down.
            //
            // The preview is drawn by ImGui with its own sampler, always with a linear
            // filter, so it does not react to the filter chosen in the Render category.
            //
            // An sRGB texture read by ImGui would give linear values, and ImGui writes
            // what it reads straight into the window: the picture would be too dark.
            // Between begin and end it is read without the decoding, so the preview
            // shows the bytes of the file.
            const auto textureId = static_cast<ImTextureID>(loaded.texture.id());
            if (isSrgb) {
                rawSampler.begin();
            }
            ImGui::Image(textureId, {previewSize, previewSize}, {0.0F, 1.0F}, {1.0F, 0.0F});
            if (isSrgb) {
                rawSampler.end();
            }
        }
    }
    page.endCard();
}

// The files that could not be loaded, if there are any.
void drawFailures(Page& page, const assets::AssetCache& assets) {
    if (assets.failedPaths().empty()) {
        return;
    }
    page.beginCard("Failed to load");
    if (page.beginBlock("failed to load missing files errors")) {
        // The red of the text is a colour of the theme (Theme.hpp), shared with the
        // list of the shader programs.
        ImGui::PushStyleColor(ImGuiCol_Text, ERROR_TEXT_COLOR);
        for (const std::filesystem::path& path : assets.failedPaths()) {
            drawFileName(path);
        }
        ImGui::PopStyleColor();
    }
    page.endCard();
}

void drawAssetsTab(Page& page, const DebugContext& context, const RawTextureSampler& rawSampler) {
    const assets::AssetCache& assets = context.assets;
    page.setPlace("Diagnostics / Assets");
    page.beginColumns();
    drawFailures(page, assets);
    drawModels(page, assets);
    page.nextColumn();
    drawTextures(page, assets, rawSampler);
    page.endColumns();
}

} // namespace

void drawDiagnosticsCategory(Page& page, const DebugContext& context,
                             const RawTextureSampler& rawSampler, DiagnosticsTab tab) {
    // While the user searches, the rows of every tab can be found.
    const bool all = page.searching();
    if (all || tab == DiagnosticsTab::FrameAndShaders) {
        drawFrameAndShadersTab(page, context);
    }
    if (all || tab == DiagnosticsTab::CollisionAndPicking) {
        drawCollisionAndPickingTab(page, context);
    }
    if (all || tab == DiagnosticsTab::Assets) {
        drawAssetsTab(page, context, rawSampler);
    }
}

} // namespace debug
