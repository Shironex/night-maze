// Maze plan of the debug window: the maze seen from above, with the crystals, the
// levers, the notes, the gate, the exit and the player on it.
// See docs/modules/game/maze-generator.md
#include "debug/MazePlan.hpp"

#include "debug/Theme.hpp"
#include "game/Interactables.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "scene/Camera.hpp"

#include <glm/glm.hpp>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace debug {

namespace {

// Sizes on the plan, in pixels: the dot of the player and the line that shows where the
// camera looks.
constexpr float PLAYER_DOT_RADIUS = 3.0F;
constexpr float HEADING_LENGTH = 10.0F;

// A crystal on the plan is a dot of this radius, and the gate a line of this thickness,
// both in pixels. The walls are 1 pixel thick, so the gate stands out among them.
constexpr float CRYSTAL_DOT_RADIUS = 2.5F;
constexpr float GATE_LINE_THICKNESS = 3.0F;

// A lever and a note on the plan are squares with this half side, in pixels.
constexpr float MOUNT_MARK_HALF_SIZE = 2.5F;

// Free pixels around the plan, so that the border walls are not drawn on the very edge
// of the reserved area.
constexpr float PLAN_PADDING = 4.0F;

} // namespace

// World x runs to the right of the screen and world z down the screen. Screen y grows
// downwards and z grows towards the south, so neither axis has to be flipped.
void drawMazePlan(const game::MazeWorld& world, const game::Round& round,
                  const game::Player& player, const scene::Camera& camera, float width) {
    // Size of the maze in metres.
    const float worldWidth = static_cast<float>(world.maze.width()) * game::CELL_SIZE;
    const float worldDepth = static_cast<float>(world.maze.height()) * game::CELL_SIZE;

    // One scale (pixels per metre) for both axes keeps the cells square. The longer side
    // of the maze decides it: that side fills the width the caller gave.
    const float availableWidth = width - 2.0F * PLAN_PADDING;
    const float scale = std::max(availableWidth, 1.0F) / std::max(worldWidth, worldDepth);

    // The top left corner of the plan on the screen. The cursor is the place where ImGui
    // would put the next widget.
    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    const ImVec2 origin{cursor.x + PLAN_PADDING, cursor.y + PLAN_PADDING};

    // Turns a point of the world (x and z, the height does not matter from above) into
    // a point on the screen. A lambda: a small function that can use scale and origin.
    const auto toScreen = [scale, origin](const glm::vec3& point) {
        return ImVec2{origin.x + point.x * scale, origin.y + point.z * scale};
    };

    // The draw list of the window takes shapes in screen coordinates. They are drawn
    // with the window and clipped to it.
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // A draw list takes a colour packed into one 32 bit number. GetColorU32 packs a
    // colour of the theme (Theme.hpp) and applies the opacity of the style to it.
    const ImU32 wallColor = ImGui::GetColorU32(PLAN_WALL_COLOR);
    const ImU32 playerColor = ImGui::GetColorU32(PLAN_PLAYER_COLOR);
    const ImU32 crystalColor = ImGui::GetColorU32(PLAN_CRYSTAL_COLOR);
    const ImU32 collectedColor = ImGui::GetColorU32(PLAN_COLLECTED_COLOR);
    const ImU32 gateColor = ImGui::GetColorU32(PLAN_GATE_COLOR);
    const ImU32 exitColor = ImGui::GetColorU32(PLAN_EXIT_COLOR);
    const ImU32 leverColor = ImGui::GetColorU32(PLAN_LEVER_COLOR);
    const ImU32 noteColor = ImGui::GetColorU32(PLAN_NOTE_COLOR);

    // A wall segment is a line from one grid corner to the next: half a wall length to
    // each side of its middle, along the axis it runs on.
    constexpr float HALF_WALL = game::WALL_LENGTH / 2.0F;
    const auto halfLineOf = [](const game::WallSegment& segment) {
        return segment.axis == game::WallAxis::AlongX ? glm::vec3{HALF_WALL, 0.0F, 0.0F}
                                                      : glm::vec3{0.0F, 0.0F, HALF_WALL};
    };
    // A wall that a pulled lever has opened is still in the list of the world (the world
    // never changes during a round). It is drawn dim: a trace of where the shortcut is.
    const std::vector<bool> opened = game::openedWallFlags(world, round);
    for (std::size_t i = 0; i < world.walls.size(); ++i) {
        const game::WallSegment& wall = world.walls[i];
        const glm::vec3 halfLine = halfLineOf(wall);
        drawList->AddLine(toScreen(wall.position - halfLine), toScreen(wall.position + halfLine),
                          opened[i] ? collectedColor : wallColor);
    }

    // The exit zone: the outline of the box that wins the round, seen from above. The
    // min and max corners of the box are its north-west and south-east corners here.
    drawList->AddRect(toScreen(world.exitZone.min), toScreen(world.exitZone.max), exitColor);

    // The gate: a thick line where it stands, in the colour of wood while it is closed
    // and dim once it has opened. A maze of one cell has none.
    if (world.hasGate) {
        const glm::vec3 halfLine = halfLineOf(world.gate);
        drawList->AddLine(
            toScreen(world.gate.position - halfLine), toScreen(world.gate.position + halfLine),
            game::gateBlocks(world, round) ? gateColor : collectedColor, GATE_LINE_THICKNESS);
    }

    // The crystals: a bright dot for one that is still there, a dim ring where one was
    // collected.
    for (const game::RoundCrystal& crystal : round.crystals) {
        const ImVec2 place = toScreen(crystal.restPosition);
        if (crystal.collected) {
            drawList->AddCircle(place, CRYSTAL_DOT_RADIUS, collectedColor);
        } else {
            drawList->AddCircleFilled(place, CRYSTAL_DOT_RADIUS, crystalColor);
        }
    }

    // The levers and the notes: small squares where they hang. A lever that is pulled
    // is drawn dim. The state of the round has one entry per lever.
    const std::vector<game::Lever>& levers = world.interactables.levers;
    for (std::size_t i = 0; i < levers.size(); ++i) {
        const bool pulled =
            i < round.interactables.leverPulled.size() && round.interactables.leverPulled[i];
        const ImVec2 place = toScreen(levers[i].position);
        drawList->AddRectFilled({place.x - MOUNT_MARK_HALF_SIZE, place.y - MOUNT_MARK_HALF_SIZE},
                                {place.x + MOUNT_MARK_HALF_SIZE, place.y + MOUNT_MARK_HALF_SIZE},
                                pulled ? collectedColor : leverColor);
    }
    for (const game::Note& note : world.interactables.notes) {
        const ImVec2 place = toScreen(note.position);
        drawList->AddRectFilled({place.x - MOUNT_MARK_HALF_SIZE, place.y - MOUNT_MARK_HALF_SIZE},
                                {place.x + MOUNT_MARK_HALF_SIZE, place.y + MOUNT_MARK_HALF_SIZE},
                                noteColor);
    }

    // The player: a dot, and a line towards where the camera looks. Yaw 0 looks north
    // (up on the plan) and grows clockwise, so the direction on the screen is
    // (sin yaw, -cos yaw): the same formula as the x and z of Camera::forward.
    const ImVec2 dot = toScreen(player.position);
    const float yaw = glm::radians(camera.yawDegrees);
    const ImVec2 headingEnd{dot.x + std::sin(yaw) * HEADING_LENGTH,
                            dot.y - std::cos(yaw) * HEADING_LENGTH};
    drawList->AddLine(dot, headingEnd, playerColor);
    drawList->AddCircleFilled(dot, PLAYER_DOT_RADIUS, playerColor);

    // The draw list does not move the cursor. Dummy is an invisible widget of the given
    // size: it reserves the area of the plan, so the card knows how tall its contents
    // are.
    ImGui::Dummy(
        {worldWidth * scale + 2.0F * PLAN_PADDING, worldDepth * scale + 2.0F * PLAN_PADDING});
}

} // namespace debug
