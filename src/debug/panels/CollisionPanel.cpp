// "Collision" debug panel: the collision boxes and spheres as lines, their counts and the
// result of the last picking ray.
// See docs/modules/scene/collision.md
#include "debug/panels/CollisionPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/Interactables.hpp"
#include "game/Interaction.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "scene/Collider.hpp"

#include <imgui.h>

namespace debug {

namespace {

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

// The result of the picking ray of the last frame: where it starts, where it points,
// what it hit and what the interaction key does.
void drawLastRay(const game::PickState& pick) {
    if (!pick.hasRay) {
        ImGui::TextUnformatted("Ray: none (cursor over a panel or outside)");
        ImGui::Text("Key E: %s", actionName(pick.action));
        return;
    }
    ImGui::Text("Ray through: %s", pick.centered ? "the middle of the picture" : "the cursor");
    ImGui::Text("origin: %.2f, %.2f, %.2f", pick.ray.origin.x, pick.ray.origin.y,
                pick.ray.origin.z);
    ImGui::Text("direction: %.3f, %.3f, %.3f", pick.ray.direction.x, pick.ray.direction.y,
                pick.ray.direction.z);

    if (pick.picked.kind == game::InteractableKind::None) {
        ImGui::Text("Hit: nothing within %.1f m", game::INTERACTION_REACH);
    } else {
        ImGui::Text("Hit: %s %d at %.2f m", kindName(pick.picked.kind),
                    static_cast<int>(pick.picked.index), pick.picked.distance);
    }
    ImGui::Text("Key E: %s", actionName(pick.action));
}

// The picking (object selection by ray casting): the result of the ray of the last
// frame and the switches of its debug view.
void drawPicking(const game::PickState& pick, game::PickDebugSettings& pickDebug) {
    ImGui::TextUnformatted("Last picking ray");
    drawLastRay(pick);

    ImGui::Checkbox("Draw pick boxes and ray", &pickDebug.drawShapes);
    // The ray leaves the eye, so from the eye it is a point. Frozen, it stays where it
    // was and can be looked at from the side.
    ImGui::Checkbox("Freeze the drawn ray", &pickDebug.freezeRay);
    ImGui::TextWrapped("Red: lever boxes. White: note boxes. Green: the ray and the box "
                       "it hit. Grey: a ray that hit nothing.");
}

} // namespace

void drawCollisionPanel(const game::MazeWorld& world, const game::Round& round,
                        game::Player& player, bool& drawColliders, const game::PickState& pick,
                        game::PickDebugSettings& pickDebug) {
    // First run only: the bottom edge of the window, right of the left column (the
    // constant is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(COLLISION_PLACEMENT);
    if (ImGui::Begin("Collision")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Draw collision shapes", &drawColliders);
        ImGui::TextWrapped("Yellow: walls, pillars. Green: player. Orange: gate. "
                           "Cyan: crystal pickup. Magenta: exit zone.");

        ImGui::Separator();
        // world.colliders holds the box of every wall first and the box of every pillar
        // after them, so the two counts are the sizes of the lists they were made from.
        // The gate is one more box while it is closed, and every wall that a lever has
        // opened is one box less (game::roundObstacles).
        const int gateBoxes = game::gateBlocks(world, round) ? 1 : 0;
        const int openedWalls = game::pulledLeverCount(round);
        ImGui::Text("Boxes: %d walls (%d opened by levers), %d pillars, %d gate",
                    static_cast<int>(world.walls.size()) - openedWalls, openedWalls,
                    static_cast<int>(world.pillars.size()), gateBoxes);
        // One pickup sphere around every crystal that is not collected yet.
        ImGui::Text("All boxes: %d, pickup spheres: %d",
                    static_cast<int>(world.colliders.size()) - openedWalls + gateBoxes,
                    static_cast<int>(round.crystals.size()) - round.collectedCount);
        ImGui::TextWrapped("Wall box: %.2f m thick (the visible wall: %.2f m)",
                           game::WALL_COLLISION_THICKNESS, game::WALL_VISUAL_THICKNESS);

        ImGui::Separator();
        // The box is computed from the position of the player in every frame, exactly as
        // the movement code does it.
        const scene::Aabb box = player.box();
        ImGui::TextUnformatted("Player box");
        ImGui::Text("min: %.2f, %.2f, %.2f", box.min.x, box.min.y, box.min.z);
        ImGui::Text("max: %.2f, %.2f, %.2f", box.max.x, box.max.y, box.max.z);

        ImGui::Separator();
        drawPicking(pick, pickDebug);
    }
    ImGui::End();
}

} // namespace debug
