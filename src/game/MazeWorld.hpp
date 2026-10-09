// MazeWorld: one generated maze with everything the game needs to draw it and walk in it.
#pragma once

#include "game/Crystals.hpp"
#include "game/Interactables.hpp"
#include "game/Maze.hpp"
#include "game/MazeLayout.hpp"
#include "game/Terrain.hpp"
#include "game/WallVariants.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace game {

/// The maze the game starts with: its size in cells and its seed.
constexpr int DEFAULT_MAZE_WIDTH = 10;
constexpr int DEFAULT_MAZE_HEIGHT = 10;
constexpr std::uint32_t DEFAULT_MAZE_SEED = 1;

/// The start cell of every maze: its north-west corner. The player starts in its centre.
constexpr MazeCell START_CELL{.x = 0, .z = 0};

/// What the next maze should be like. The debug UI edits the numbers and sets
/// regenerate. The application reads the request at the start of the next frame, builds
/// the maze and clears the flag. A plain struct: the panel only writes data, and the
/// application decides when the maze is replaced.
struct MazeSettings {
    /// Number of columns (cells along X) and of rows (cells along Z).
    int width = DEFAULT_MAZE_WIDTH;
    int height = DEFAULT_MAZE_HEIGHT;

    /// Seed of the generator: the same size and seed always give the same maze.
    std::uint32_t seed = DEFAULT_MAZE_SEED;

    /// How many crystals the maze should get: a number, or CRYSTAL_COUNT_FROM_SIZE for
    /// one crystal per CELLS_PER_CRYSTAL cells. A new game writes the number of its
    /// difficulty level here (game/Difficulty.hpp).
    int crystalCount = CRYSTAL_COUNT_FROM_SIZE;

    /// How many levers and notes the maze should get (see game/Interactables.hpp). They
    /// are placed when the maze is built, so a new number shows after "regenerate" too.
    InteractableSettings interactables;

    /// Whether the worn walls are drawn with their own textures (MazeWorld::wallVariants).
    /// Unlike the fields above it needs no new maze: switched off, every wall is drawn
    /// plain from the next frame on.
    bool wallVariants = true;

    /// True when a new maze was asked for and has not been built yet.
    bool regenerate = false;
};

/// A maze placed in the world, standing on its terrain. Everything here is computed
/// when the maze is generated (and again when the height scale of the terrain changes),
/// and only read in between: the frame loop does no layout work.
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

    /// The ground the maze stands on, with the land around it. Flat until placeOnTerrain
    /// has run. Everything below that has a height takes it from here.
    Terrain terrain;

    /// Every wall segment and the position of every pillar (see MazeLayout.hpp). The y
    /// of each is the lowest ground under its footprint: a wall is a straight block on
    /// uneven ground, so it is sunk until no part of its base is above the ground and no
    /// gap shows under it.
    std::vector<WallSegment> walls;
    std::vector<glm::vec3> pillars;

    /// Model matrices, one per object to draw: a wall model for every segment (same
    /// order as walls) and a pillar model for every pillar.
    std::vector<glm::mat4> wallMatrices;
    std::vector<glm::mat4> pillarMatrices;

    /// The obstacles that never change: the box of every wall, then of every pillar, then
    /// the one box of the stile (game::stileBox). The gate is not in this list, because
    /// it stops being an obstacle when it opens: the obstacle list of a round is built by
    /// game::roundObstacles.
    std::vector<scene::Aabb> colliders;

    /// The number of the wall in walls that carries the stile of the start cell
    /// (game::stileWallIndex): it is drawn with the stile models in place of the wall
    /// model, has the last box of colliders in front of it and bare ground at its foot.
    /// Every built world has one. Empty, the world has no stile at all.
    std::optional<std::size_t> stileWall;

    /// Where the player starts: the centre of cell (0, 0), feet on the ground.
    glm::vec3 startPosition{0.0F};

    /// Camera yaw at the start, in degrees: towards the first open side of the start cell.
    float startYawDegrees = 0.0F;

    /// The exit cell: the cell farthest from the start, counted in passages walked
    /// (game::farthestCell), and its centre on the ground.
    MazeCell exitCell;
    glm::vec3 exitPosition{0.0F};

    /// The gate across the open side of the exit cell, lowered to the ground like a wall.
    /// hasGate is false only in a maze of one cell, whose exit cell has no open side:
    /// gate and gateBox mean nothing then.
    bool hasGate = false;
    WallSegment gate;
    /// The collision box of the closed gate: the same box a wall segment there would have.
    scene::Aabb gateBox;

    /// The box in the middle of the exit cell that wins the round when the player enters
    /// it while the gate is open (game::exitZone). It stands on the ground.
    scene::Aabb exitZone;

    /// The crystals of the maze, chosen from its seed (game::placeCrystals). Which of
    /// them are already collected is not stored here: that is the state of a round.
    std::vector<CrystalSpawn> crystals;

    /// The dead end of the heartstone (game::heartstoneCell), or empty in a maze that is
    /// too small to have one. It is not one of the crystals above. Whether it is taken
    /// is the state of a round.
    std::optional<MazeCell> heartstone;

    /// The crystals as the seed alone gives them, before the heartstone took its dead
    /// end. The levers, the notes and the puddles are placed around THESE cells and not
    /// around crystals: so the heartstone moved none of them in any seed. It differs from
    /// crystals by the one or two crystals that made room (buildMazeWorld).
    std::vector<CrystalSpawn> seedCrystals;

    /// The levers and the notes of the maze, chosen from its seed
    /// (game::placeInteractables) and hanging at the height of the terrain. Which levers
    /// are pulled is not stored here: that is the state of a round.
    Interactables interactables;

    /// For every lever, in the order of Interactables::levers: the number of the wall it
    /// opens in walls. The same number finds the matrix of that wall in wallMatrices and
    /// its box in colliders, because the three lists are in the same order.
    std::vector<std::size_t> leverWalls;

    /// The look of every wall (plain, painted or shaped), chosen from the seed
    /// (game::chooseWallVariants). Same order as walls.
    std::vector<WallVariant> wallVariants;
};

/// Camera yaw, in degrees, that looks in the given direction: 0 for North (-Z), 90 for
/// East (+X), 180 for South (+Z) and 270 for West (-X). See scene::Camera::yawDegrees.
float yawTowards(Direction direction);

/// Model matrix of a wall segment: the wall model moved to the position of the segment
/// and, for a segment along Z, turned a quarter around the vertical axis. The gate uses
/// it too: its model follows the same convention as the wall model.
glm::mat4 wallModelMatrix(const WallSegment& segment);

/// How far the footprint of a wall, a pillar or the gate reaches past its collision box
/// on every side when the ground under it is looked up, in metres. The models are a
/// little wider than their boxes at the base (the foot of a pillar is 0.4 m wide, its
/// box 0.3 m), and the whole model has to be covered.
constexpr float FOOTPRINT_MARGIN = 0.05F;

/// The height of the ground at the centre of a cell of the world.
float groundHeightAt(const MazeWorld& world, MazeCell cell);

/// Builds the terrain of the world from the heightmap and puts everything on it: the
/// walls, the pillars and the gate are lowered to the lowest ground under them, their
/// matrices and collision boxes follow (the box of the stile with its wall), and the start, the
/// exit, the exit zone, the levers and the notes are moved to the height of the ground. Nothing
/// moves sideways, so the plan of the maze and every collision in the horizontal plane stay as they
/// were.
///
/// It can be called again on the same world with another height scale. Things that copy
/// heights out of the world (the crystals and the obstacle list of a round, the player)
/// have to be updated by the caller afterwards.
void placeOnTerrain(MazeWorld& world, const Heightmap& heightmap, float heightScale);

/// Generates a maze (game::generateMaze) and computes everything else in MazeWorld
/// from it, on flat ground at y = 0. interactables says how many levers and notes are
/// wanted: left out, the maze gets the default numbers. crystalCount says how many
/// crystals are wanted (game::placeCrystals): left out, the size of the maze decides.
/// Throws std::invalid_argument for a size that Maze does not accept.
MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed,
                         const InteractableSettings& interactables = {},
                         int crystalCount = CRYSTAL_COUNT_FROM_SIZE);

/// The same, with the maze standing on the terrain made from the heightmap
/// (placeOnTerrain).
MazeWorld buildMazeWorld(int width, int height, std::uint32_t seed, const Heightmap& heightmap,
                         float heightScale, const InteractableSettings& interactables = {},
                         int crystalCount = CRYSTAL_COUNT_FROM_SIZE);

} // namespace game
