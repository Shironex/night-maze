// "Collision" debug panel: the collision boxes as lines, their counts and the noclip mode.
// See docs/modules/scene/collision.md
#include "debug/panels/CollisionPanel.hpp"

#include "debug/PanelLayout.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "scene/Collider.hpp"

#include <imgui.h>

namespace debug {

void drawCollisionPanel(const game::MazeWorld& world, game::Player& player, bool& drawColliders) {
    // First run only: the bottom edge of the window, right of the left column (the
    // constant is in PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(COLLISION_PLACEMENT);
    if (ImGui::Begin("Collision")) {
        // Checkbox reads and writes a bool through the pointer.
        ImGui::Checkbox("Draw collision boxes", &drawColliders);
        ImGui::TextWrapped("Yellow: walls and pillars. Green: the player.");

        // The same switch as the N key. In noclip mode the boxes below are ignored.
        ImGui::Checkbox("Noclip (key N)", &player.noclip);

        ImGui::Separator();
        // world.colliders holds the box of every wall first and the box of every pillar
        // after them, so the two counts are the sizes of the lists they were made from.
        ImGui::Text("Wall boxes: %d", static_cast<int>(world.walls.size()));
        ImGui::Text("Pillar boxes: %d", static_cast<int>(world.pillars.size()));
        ImGui::Text("All boxes: %d", static_cast<int>(world.colliders.size()));
        ImGui::TextWrapped("Wall box: %.2f m thick (the visible wall: %.2f m)",
                           game::WALL_COLLISION_THICKNESS, game::WALL_VISUAL_THICKNESS);

        ImGui::Separator();
        // The box is computed from the position of the player in every frame, exactly as
        // the movement code does it.
        const scene::Aabb box = player.box();
        ImGui::TextUnformatted("Player box");
        ImGui::Text("min: %.2f, %.2f, %.2f", box.min.x, box.min.y, box.min.z);
        ImGui::Text("max: %.2f, %.2f, %.2f", box.max.x, box.max.y, box.max.z);
    }
    ImGui::End();
}

} // namespace debug
