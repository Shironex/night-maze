// "Maze" debug panel: size and seed of the maze, regeneration and a plan seen from above.
// See docs/modules/game/maze-generator.md
#include "debug/panels/MazePanel.hpp"

#include "game/MazeLayout.hpp"
#include "game/MazeWorld.hpp"
#include "game/Player.hpp"
#include "scene/Camera.hpp"

#include <glm/glm.hpp>

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

namespace debug {

namespace {

// Where the panel appears and how big it is the first time the program runs (later ImGui
// remembers it in imgui.ini): the top of the right edge of a 1280 x 720 window.
constexpr ImVec2 FIRST_POSITION{970.0F, 10.0F};
constexpr ImVec2 FIRST_SIZE{300.0F, 430.0F};

// Limits of the size sliders, in cells. The game draws every floor tile, wall and pillar
// with its own draw call, about three per cell, so a much larger maze would make the
// frame slow. game::Maze itself accepts up to Maze::MAX_SIZE.
constexpr int MIN_MAZE_SIZE = 2;
constexpr int MAX_MAZE_SIZE = 40;

// The seed field changes by this much for one click on its + or - button.
constexpr std::uint32_t SEED_STEP = 1;

// Colours of the plan (red, green, blue, alpha, each 0 to 255).
constexpr ImU32 WALL_COLOR = IM_COL32(210, 210, 210, 255);
constexpr ImU32 PLAYER_COLOR = IM_COL32(80, 255, 120, 255);

// Sizes on the plan, in pixels: the dot of the player and the line that shows where the
// camera looks.
constexpr float PLAYER_DOT_RADIUS = 3.0F;
constexpr float HEADING_LENGTH = 10.0F;

// Free pixels around the plan, so that the border walls are not drawn on the very edge
// of the reserved area.
constexpr float PLAN_PADDING = 4.0F;

// Draws the walls of the maze as seen from above, north (-Z) at the top, with the player
// on it. World x runs to the right of the screen and world z down the screen. Screen y
// grows downwards and z grows towards the south, so neither axis has to be flipped.
void drawPlan(const game::MazeWorld& world, const game::Player& player,
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

    // A wall segment is a line from one grid corner to the next: half a wall length to
    // each side of its middle, along the axis it runs on.
    constexpr float HALF_WALL = game::WALL_LENGTH / 2.0F;
    for (const game::WallSegment& wall : world.walls) {
        const glm::vec3 halfLine = wall.axis == game::WallAxis::AlongX
                                       ? glm::vec3{HALF_WALL, 0.0F, 0.0F}
                                       : glm::vec3{0.0F, 0.0F, HALF_WALL};
        drawList->AddLine(toScreen(wall.position - halfLine), toScreen(wall.position + halfLine),
                          WALL_COLOR);
    }

    // The player: a dot, and a line towards where the camera looks. Yaw 0 looks north
    // (up on the plan) and grows clockwise, so the direction on the screen is
    // (sin yaw, -cos yaw): the same formula as the x and z of Camera::forward.
    const ImVec2 dot = toScreen(player.position);
    const float yaw = glm::radians(camera.yawDegrees);
    const ImVec2 headingEnd{dot.x + std::sin(yaw) * HEADING_LENGTH,
                            dot.y - std::cos(yaw) * HEADING_LENGTH};
    drawList->AddLine(dot, headingEnd, PLAYER_COLOR);
    drawList->AddCircleFilled(dot, PLAYER_DOT_RADIUS, PLAYER_COLOR);

    // The draw list does not move the cursor. Dummy is an invisible widget of the given
    // size: it reserves the area of the plan, so the panel knows how tall its contents
    // are and scrolls correctly.
    ImGui::Dummy(
        {worldWidth * scale + 2.0F * PLAN_PADDING, worldDepth * scale + 2.0F * PLAN_PADDING});
}

} // namespace

void drawMazePanel(game::MazeSettings& settings, const game::MazeWorld& world,
                   const game::Player& player, const scene::Camera& camera) {
    ImGui::SetNextWindowPos(FIRST_POSITION, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(FIRST_SIZE, ImGuiCond_FirstUseEver);
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

        drawPlan(world, player, camera);
    }
    ImGui::End();
}

} // namespace debug
