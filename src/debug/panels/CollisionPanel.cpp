// "Collision" debug panel: the collision boxes as lines, their counts and the noclip mode.
// See docs/modules/scene/collision.md
#include "debug/panels/CollisionPanel.hpp"

#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "scene/Collider.hpp"

#include <imgui.h>

namespace debug {

namespace {

// Where the panel appears and how big it is the first time the program runs (later ImGui
// remembers it in imgui.ini): the bottom of the right edge of a 1280 x 720 window.
constexpr ImVec2 FIRST_POSITION{970.0F, 450.0F};
constexpr ImVec2 FIRST_SIZE{300.0F, 260.0F};

} // namespace

void drawCollisionPanel(const game::MazeWorld& world, game::Player& player, bool& drawColliders) {
    ImGui::SetNextWindowPos(FIRST_POSITION, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(FIRST_SIZE, ImGuiCond_FirstUseEver);
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
