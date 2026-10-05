// MazeWorld: one generated maze with everything the game needs to draw it and walk in it.
// See docs/modules/game/maze-rendering.md
#pragma once

#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace game {

/// The maze the game starts with: its size in cells and its seed.
constexpr int DEFAULT_MAZE_WIDTH = 10;
constexpr int DEFAULT_MAZE_HEIGHT = 10;
constexpr std::uint32_t DEFAULT_MAZE_SEED = 1;

/// What the next maze should be like. The debug UI edits the three numbers and sets
/// regenerate. The application reads the request at the start of the next frame, builds
/// the maze and clears the flag. A plain struct: the panel only writes data, and the
/// application decides when the maze is replaced.
struct MazeSettings {
    /// Number of columns (cells along X) and of rows (cells along Z).
    int width = DEFAULT_MAZE_WIDTH;
    int height = DEFAULT_MAZE_HEIGHT;

    /// Seed of the generator: the same size and seed always give the same maze.
    std::uint32_t seed = DEFAULT_MAZE_SEED;

    /// True when a new maze was asked for and has not been built yet.
    bool regenerate = false;
};

/// A maze placed in the world. Everything here is computed once, when the maze is
/// generated, and only read afterwards: the frame loop does no layout work.
///
/// Plain data without OpenGL, so tests can build and inspect it.
struct MazeWorld {
    /// Takes the maze. Maze has no default constructor (a maze without a size makes no
    /// sense), so a MazeWorld cannot be created empty either. The other fields start
    /// empty: buildMazeWorld fills them.
    explicit MazeWorld(Maze generatedMaze) : maze(std::move(generatedMaze)) {}

    /// The grid of cells and walls.
    Maze maze;

    /// The seed the maze was generated from.
    std::uint32_t seed = 0;

    /// Every wall segment and the position of every pillar (see MazeLayout.hpp).
    std::vector<WallSegment> walls;
    std::vector<glm::vec3> pillars;

    /// Model matrices, one per object to draw: a floor tile for every cell, a wall model
    /// for every segment (same order as walls), a pillar model for every pillar.
    std::vector<glm::mat4> floorMatrices;
    std::vector<glm::mat4> wallMatrices;
    std::vector<glm::mat4> pillarMatrices;

    /// The obstacle list for the player: the box of every wall, then of every pillar.
    std::vector<scene::Aabb> colliders;

    /// Where the point lights of the maze hang: in its dead ends, at most
    /// scene::MAX_POINT_LIGHTS of them (see deadEndLightPositions in game/Lighting.hpp).
    std::vector<glm::vec3> pointLightPositions;

    /// Where the player starts: the centre of cell (0, 0), feet on the floor.
    glm::vec3 startPosition{0.0F};

    /// Camera yaw at the start, in degrees: towards the first open side of the start cell.
    float startYawDegrees = 0.0F;

    /// The centre of the far corner cell (width - 1, height - 1) at floor level: the
    /// place of the future exit.
    glm::vec3 exitPosition{0.0F};
};

/// Camera yaw, in degrees, that looks in the given direction: 0 for North (-Z), 90 for
/// East (+X), 180 for South (+Z) and 270 for West (-X). See scene::Camera::yawDegrees.
float yawTowards(Direction direction);

/// Generates a maze (game::generateMaze) and computes everything else in MazeWorld
/// from it. Throws std::invalid_argument for a size that Maze does not accept.
MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed);

} // namespace game
