// "Maze" debug panel: size and seed of the maze, regeneration and a plan seen from above
// with the crystals, the gate and the exit on it.
// See docs/modules/game/maze-generator.md
#include "debug/panels/MazePanel.hpp"

#include "debug/PanelLayout.hpp"
#include "debug/Theme.hpp"
#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "game/Round.hpp"
#include "scene/Camera.hpp"

#include <glm/glm.hpp>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace debug {

namespace {

// Limits of the size sliders, in cells. The game draws every wall and pillar with its
// own draw call, about two per cell, and the terrain has 32 triangles per cell, so a much
// larger maze would make the frame slow. game::Maze itself accepts up to Maze::MAX_SIZE.
constexpr int MIN_MAZE_SIZE = 2;
constexpr int MAX_MAZE_SIZE = 40;

// The seed field changes by this much for one click on its + or - button.
constexpr std::uint32_t SEED_STEP = 1;

// Sizes on the plan, in pixels: the dot of the player and the line that shows where the
// camera looks.
constexpr float PLAYER_DOT_RADIUS = 3.0F;
constexpr float HEADING_LENGTH = 10.0F;

// A crystal on the plan is a dot of this radius, and the gate a line of this thickness,
// both in pixels. The walls are 1 pixel thick, so the gate stands out among them.
constexpr float CRYSTAL_DOT_RADIUS = 2.5F;
constexpr float GATE_LINE_THICKNESS = 3.0F;

// Free pixels around the plan, so that the border walls are not drawn on the very edge
// of the reserved area.
constexpr float PLAN_PADDING = 4.0F;

// Draws the walls of the maze as seen from above, north (-Z) at the top, with the gate,
// the exit zone, the crystals and the player on it. World x runs to the right of the
// screen and world z down the screen. Screen y grows downwards and z grows towards the
// south, so neither axis has to be flipped.
void drawPlan(const game::MazeWorld& world, const game::Round& round, const game::Player& player,
              const scene::Camera& camera) {
    // Size of the maze in metres.
    const float worldWidth = static_cast<float>(world.maze.width()) * game::CELL_SIZE;
    const float worldDepth = static_cast<float>(world.maze.height()) * game::CELL_SIZE;

    // The plan is as wide as the panel allows. One scale (pixels per metre) for both axes
    // keeps the cells square. The longer side of the maze decides it.
    const float availableWidth = ImGui::GetContentRegionAvail().x - 2.0F * PLAN_PADDING;
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

    // The draw list of the panel takes shapes in screen coordinates. They are drawn
    // with the panel and clipped to it.
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // A draw list takes a colour packed into one 32 bit number. GetColorU32 packs a
    // colour of the theme (Theme.hpp) and applies the opacity of the style to it.
    const ImU32 wallColor = ImGui::GetColorU32(PLAN_WALL_COLOR);
    const ImU32 playerColor = ImGui::GetColorU32(PLAN_PLAYER_COLOR);
    const ImU32 crystalColor = ImGui::GetColorU32(PLAN_CRYSTAL_COLOR);
    const ImU32 collectedColor = ImGui::GetColorU32(PLAN_COLLECTED_COLOR);
    const ImU32 gateColor = ImGui::GetColorU32(PLAN_GATE_COLOR);
    const ImU32 exitColor = ImGui::GetColorU32(PLAN_EXIT_COLOR);

    // A wall segment is a line from one grid corner to the next: half a wall length to
    // each side of its middle, along the axis it runs on.
    constexpr float HALF_WALL = game::WALL_LENGTH / 2.0F;
    const auto halfLineOf = [](const game::WallSegment& segment) {
        return segment.axis == game::WallAxis::AlongX ? glm::vec3{HALF_WALL, 0.0F, 0.0F}
                                                      : glm::vec3{0.0F, 0.0F, HALF_WALL};
    };
    for (const game::WallSegment& wall : world.walls) {
        const glm::vec3 halfLine = halfLineOf(wall);
        drawList->AddLine(toScreen(wall.position - halfLine), toScreen(wall.position + halfLine),
                          wallColor);
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
    // size: it reserves the area of the plan, so the panel knows how tall its contents
    // are and scrolls correctly.
    ImGui::Dummy(
        {worldWidth * scale + 2.0F * PLAN_PADDING, worldDepth * scale + 2.0F * PLAN_PADDING});
}

} // namespace

void drawMazePanel(game::MazeSettings& settings, const game::MazeWorld& world,
                   const game::Round& round, const game::Player& player,
                   const scene::Camera& camera) {
    // First run only: the top right corner of the window (the constant is in
    // PanelLayout.hpp). Later ImGui remembers the panel in imgui.ini.
    placePanelOnFirstUse(MAZE_PLACEMENT);
    if (ImGui::Begin("Maze")) {
        // The three widgets edit the request, not the maze: nothing happens until one of
        // the buttons sets settings.regenerate.
        ImGui::SliderInt("Width", &settings.width, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells",
                         ImGuiSliderFlags_AlwaysClamp);
        ImGui::SliderInt("Height", &settings.height, MIN_MAZE_SIZE, MAX_MAZE_SIZE, "%d cells",
                         ImGuiSliderFlags_AlwaysClamp);
        // InputScalar edits a number of any type through a pointer: the type is named by
        // the second argument and must match the variable, here a 32 bit unsigned.
        ImGui::InputScalar("Seed", ImGuiDataType_U32, &settings.seed, &SEED_STEP);

        if (ImGui::Button("Regenerate")) {
            settings.regenerate = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Random seed")) {
            // std::random_device asks the operating system for a number that cannot be
            // predicted. It only picks the seed: the maze itself is still built by the
            // seeded generator, so writing the seed down brings the same maze back.
            std::random_device device;
            settings.seed = static_cast<std::uint32_t>(device());
            settings.regenerate = true;
        }

        ImGui::Separator();
        // The maze in play, which may differ from the request above until a button
        // is clicked.
        ImGui::Text("In play: %d x %d cells, seed %u", world.maze.width(), world.maze.height(),
                    static_cast<unsigned int>(world.seed));
        ImGui::Text("Walls: %d, pillars: %d", static_cast<int>(world.walls.size()),
                    static_cast<int>(world.pillars.size()));
        // The crystals are chosen from the seed together with the maze. How many of them
        // open the gate is a rule of the round (the Gameplay panel).
        ImGui::Text("Crystals: %d, exit in cell (%d, %d)", static_cast<int>(world.crystals.size()),
                    world.exitCell.x, world.exitCell.z);

        drawPlan(world, round, player, camera);
    }
    ImGui::End();
}

} // namespace debug
