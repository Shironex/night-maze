// InteractableRenderer: draws the levers and the notes of a maze with their models.
#include "game/InteractableRenderer.hpp"

#include "assets/AssetCache.hpp"
#include "core/Paths.hpp"
#include "game/Interaction.hpp"
#include "game/MazeWorld.hpp"
#include "game/ModelDraw.hpp"
#include "game/Round.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace game {

namespace {

// Model files, relative to the assets directory. All three have their origin in the
// middle of their back, the point fixed to the wall, and stand out along +Z (see
// game/Interaction.hpp). The handle is a model of its own, because it is the one part
// that moves: its origin is the pivot it turns around.
constexpr const char* LEVER_PLATE_MODEL_FILE = "models/lever.obj";
constexpr const char* LEVER_HANDLE_MODEL_FILE = "models/lever_handle.obj";
constexpr const char* NOTE_MODEL_FILE = "models/note.obj";

// Draws a model once. drawModel takes a list of matrices. A span made of a pointer and
// a count of 1 is a list with this one matrix in it.
void drawOne(const gfx::Shader& shader, const assets::LoadedModel* model, const glm::mat4& matrix) {
    drawModel(shader, model, std::span<const glm::mat4>(&matrix, 1));
}

} // namespace

InteractableRenderer::InteractableRenderer(assets::AssetCache& assets)
    : m_leverPlate(assets.model(core::assetPath(LEVER_PLATE_MODEL_FILE))),
      m_leverHandle(assets.model(core::assetPath(LEVER_HANDLE_MODEL_FILE))),
      m_note(assets.model(core::assetPath(NOTE_MODEL_FILE))) {}

void InteractableRenderer::draw(const gfx::Shader& shader, const MazeWorld& world,
                                const Round& round, const PickState& pick,
                                const glm::vec3& highlight) const {
    setModelSamplers(shader);

    // Iron and paper give off no light. Only the picked one glows: the highlight.
    constexpr glm::vec3 NO_GLOW{0.0F};

    const std::vector<Lever>& levers = world.interactables.levers;
    for (std::size_t i = 0; i < levers.size(); ++i) {
        // uEmissive is set for every lever, picked or not: a uniform keeps its value
        // from one draw call to the next, and the lever before may have been the
        // picked one.
        const bool picked = pick.action == Interaction::PullLever && pick.picked.index == i;
        shader.setVec3(EMISSIVE_UNIFORM, picked ? highlight : NO_GLOW);

        // The plate hangs still. The handle is the same lever seen through a matrix of
        // its own, which tilts it by how far the pull has come.
        drawOne(shader, m_leverPlate, mountModelMatrix(levers[i].position, levers[i].mount.side));
        drawOne(shader, m_leverHandle, leverHandleMatrix(levers[i], leverHandleProgress(round, i)));
    }

    const std::vector<Note>& notes = world.interactables.notes;
    for (std::size_t i = 0; i < notes.size(); ++i) {
        const bool picked = pick.action == Interaction::ReadNote && pick.picked.index == i;
        shader.setVec3(EMISSIVE_UNIFORM, picked ? highlight : NO_GLOW);
        drawOne(shader, m_note, mountModelMatrix(notes[i].position, notes[i].mount.side));
    }

    // Back to black for whatever is drawn next with this program.
    shader.setVec3(EMISSIVE_UNIFORM, NO_GLOW);
}

} // namespace game
