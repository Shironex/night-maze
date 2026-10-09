// InteractableRenderer: draws the levers and the notes of a maze with their models, the
// chalk crook beside every lever and the iron ring on the wall every lever opens.
#include "game/InteractableRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/Interaction.hpp"
#include "game/MazeWorld.hpp"
#include "game/ModelDraw.hpp"
#include "game/Round.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace game {

namespace {

// Model files, relative to the assets directory. All of them have their origin in the
// middle of their back, the point fixed to the wall, and stand out along +Z (see
// game/Interaction.hpp). The handle is a model of its own, because it is the one part
// that moves: its origin is the pivot it turns around. The rope is two models, of which
// one is drawn: it hangs slack until the handle is down and is pulled straight then.
constexpr const char* LEVER_POST_MODEL_FILE = "models/crook_post.obj";
constexpr const char* LEVER_HANDLE_MODEL_FILE = "models/crook_handle.obj";
constexpr const char* LEVER_ROPE_SLACK_MODEL_FILE = "models/crook_rope_slack.obj";
constexpr const char* LEVER_ROPE_TAUT_MODEL_FILE = "models/crook_rope_taut.obj";
// The iron ring at the foot of a wall that a lever opens: the other end of the rope.
constexpr const char* SLAB_RING_MODEL_FILE = "models/slab_ring.obj";
// The chalk marks: flat strokes 3 mm in front of the wall (tools/blender/build_chalk.py).
// A note is a lamp (a line of the story) or an arrow (a hint), the crook marks a lever.
constexpr const char* CHALK_LAMP_MODEL_FILE = "models/chalk_lamp.obj";
constexpr const char* CHALK_ARROW_MODEL_FILE = "models/chalk_arrow.obj";
constexpr const char* CHALK_CROOK_MODEL_FILE = "models/chalk_crook.obj";

// Draws a model once. drawModel takes a list of matrices. A span made of a pointer and
// a count of 1 is a list with this one matrix in it.
void drawOne(const gfx::Shader& shader, const assets::LoadedModel* model, const glm::mat4& matrix) {
    drawModel(shader, model, std::span<const glm::mat4>(&matrix, 1));
}

} // namespace

InteractableRenderer::InteractableRenderer(assets::AssetCache& assets)
    : m_leverPost(assets.model(core::assetPath(LEVER_POST_MODEL_FILE))),
      m_leverHandle(assets.model(core::assetPath(LEVER_HANDLE_MODEL_FILE))),
      m_leverRopeSlack(assets.model(core::assetPath(LEVER_ROPE_SLACK_MODEL_FILE))),
      m_leverRopeTaut(assets.model(core::assetPath(LEVER_ROPE_TAUT_MODEL_FILE))),
      m_slabRing(assets.model(core::assetPath(SLAB_RING_MODEL_FILE))),
      m_chalkLamp(assets.model(core::assetPath(CHALK_LAMP_MODEL_FILE))),
      m_chalkArrow(assets.model(core::assetPath(CHALK_ARROW_MODEL_FILE))),
      m_chalkCrook(assets.model(core::assetPath(CHALK_CROOK_MODEL_FILE))) {}

void InteractableRenderer::draw(const gfx::Shader& shader, const MazeWorld& world,
                                const Round& round, const PickState& pick,
                                const glm::vec3& highlight) const {
    setModelSamplers(shader);

    // Wood, rope and iron give off no light. Only the picked lever glows: the highlight.
    constexpr glm::vec3 NO_GLOW{0.0F};

    const std::vector<Lever>& levers = world.interactables.levers;
    for (std::size_t i = 0; i < levers.size(); ++i) {
        // uEmissive is set for every lever, picked or not: a uniform keeps its value
        // from one draw call to the next, and the lever before may have been the
        // picked one.
        const bool picked = pick.action == Interaction::PullLever && pick.picked.index == i;
        shader.setVec3(EMISSIVE_UNIFORM, picked ? highlight : NO_GLOW);

        // The board hangs still, and the rope with it. The handle is the same lever
        // seen through a matrix of its own, which tilts it by how far the pull has come.
        const glm::mat4 onWall = mountModelMatrix(levers[i].position, levers[i].mount.side);
        const float pulled = leverHandleProgress(round, i);
        drawOne(shader, m_leverPost, onWall);
        drawOne(shader, leverRopeTaut(pulled) ? m_leverRopeTaut : m_leverRopeSlack, onWall);
        drawOne(shader, m_leverHandle, leverHandleMatrix(levers[i], pulled));
    }

    // The ring on both faces of the wall each lever opens. It is a sign and not a thing
    // to use, so it never glows, and it goes down with its wall.
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
    for (std::size_t i = 0; i < levers.size(); ++i) {
        const std::array<glm::mat4, 2> rings = slabRingMatrices(world, round, i);
        drawModel(shader, m_slabRing, std::span<const glm::mat4>(rings));
    }

    // The crook beside every lever: a sign, not a thing to use, so it is never the
    // picked one. It lies on the same wall face, moved sideways.
    shader.setVec3(EMISSIVE_UNIFORM, CHALK_GLOW);
    for (const Lever& lever : levers) {
        drawOne(shader, m_chalkCrook,
                glm::translate(mountModelMatrix(lever.position, lever.mount.side),
                               {CHALK_CROOK_OFFSET, 0.0F, 0.0F}));
    }

    // A hint leans towards what it names now, so the crystals that are left are asked
    // for in every frame, like for the text of the card.
    const std::vector<MazeCell> crystalCells = remainingCrystalCells(world, round);

    const std::vector<Note>& notes = world.interactables.notes;
    for (std::size_t i = 0; i < notes.size(); ++i) {
        const bool picked = pick.action == Interaction::ReadNote && pick.picked.index == i;
        shader.setVec3(EMISSIVE_UNIFORM, picked ? highlight : CHALK_GLOW);

        const glm::mat4 onWall = mountModelMatrix(notes[i].position, notes[i].mount.side);
        const std::optional<Compass> lean = noteLean(notes[i], world.exitCell, crystalCells);
        if (!lean) {
            // A line of the story, or a hint with nothing left to point at.
            drawOne(shader, m_chalkLamp, onWall);
            continue;
        }
        // The arrow is turned around the axis that stands on the wall (+Z of the model).
        const float turn = glm::radians(chalkArrowDegrees(*lean, notes[i].mount.side));
        drawOne(shader, m_chalkArrow, glm::rotate(onWall, turn, {0.0F, 0.0F, 1.0F}));
    }

    // Back to black for whatever is drawn next with this program.
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
}

} // namespace game
