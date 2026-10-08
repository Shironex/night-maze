// GateLamp: the lantern of the exit gate (how bright it is and in which colour) and where
// the gatehouse, its three lanterns, its light and the milestone stand.
#include "game/GateLamp.hpp"

#include "game/Interactables.hpp"
#include "game/MazeLayout.hpp"
#include "scene/Transform.hpp"

#include <algorithm>
#include <cmath>

namespace game {

namespace {

// The direction a side of a cell lies in, seen from the middle of the cell, as a vector
// of length 1 on the ground: North is -Z, East is +X.
glm::vec3 directionVector(Direction side) {
    return {static_cast<float>(columnStep(side)), 0.0F, static_cast<float>(rowStep(side))};
}

// The side to the left of somebody who looks in the given direction. The enum lists the
// directions clockwise, so one step back in it is a quarter turn to the left. Three steps
// forward are the same, and stay inside the four numbers.
Direction leftOf(Direction direction) {
    return static_cast<Direction>((static_cast<int>(direction) + DIRECTION_COUNT - 1) %
                                  DIRECTION_COUNT);
}

// Model matrix of something that stands upright at position, scale times as large as its
// model.
glm::mat4 placedAt(const glm::vec3& position, float scale) {
    scene::Transform transform;
    transform.position = position;
    transform.scale = glm::vec3{scale};
    return transform.matrix();
}

// True when a stone may stand against this wall of the approach cell: the maze has the
// wall, no lever opens it, and no lever and no note hangs on it.
bool wallTakesMilestone(const MazeWorld& world, const WallRef& wall) {
    if (!world.maze.hasWall(wall.cell.x, wall.cell.z, wall.side)) {
        return false;
    }
    for (const Lever& lever : world.interactables.levers) {
        if (sameWall(lever.opens, wall) || sameWall(lever.mount, wall)) {
            return false;
        }
    }
    return std::ranges::none_of(world.interactables.notes,
                                [&wall](const Note& note) { return sameWall(note.mount, wall); });
}

} // namespace

float gateLampStrength(int collected, int needed, float gateProgress,
                       const GateSettings& settings) {
    // The two ends of the ember, in order and inside the range of a strength.
    const float emberLow = std::clamp(std::min(settings.emberMin, settings.emberMax), 0.0F, 1.0F);
    const float emberHigh = std::clamp(std::max(settings.emberMin, settings.emberMax), 0.0F, 1.0F);

    // How much of what the gate asks for is collected, from 0 to 1. A gate that asks for
    // nothing has all of it.
    float share = 1.0F;
    if (needed > 0) {
        share = std::clamp(static_cast<float>(collected) / static_cast<float>(needed), 0.0F, 1.0F);
    }
    const float ember = emberLow + (emberHigh - emberLow) * share;

    // From the ember to full while the gate sinks. At progress 1 the product with ember
    // is exactly 0, so the result is exactly 1.
    const float open = std::clamp(gateProgress, 0.0F, 1.0F);
    return ember * (1.0F - open) + open;
}

glm::vec3 gateLampColor(float gateProgress) {
    return glm::mix(GATE_LAMP_COLD_COLOR, GATE_LAMP_WARM_COLOR,
                    std::clamp(gateProgress, 0.0F, 1.0F));
}

PointLightSpot gateLampLight(const glm::vec3& position, float strength, float gateProgress,
                             const GateSettings& settings) {
    return {.position = position,
            .strength = strength,
            .ownLook = true,
            .color = gateLampColor(gateProgress),
            .radius = settings.lightRadius,
            .intensity = GATE_LAMP_LIGHT_INTENSITY};
}

Direction gateSide(const MazeWorld& world) {
    // The gate stands on the border of the exit cell, one metre from its middle. The
    // side it is on is the direction that step points in: its larger part, and the sign
    // of that part.
    const glm::vec3 center = cellCenter(world.exitCell.x, world.exitCell.z);
    const float stepX = world.gate.position.x - center.x;
    const float stepZ = world.gate.position.z - center.z;
    if (std::abs(stepX) > std::abs(stepZ)) {
        return stepX > 0.0F ? Direction::East : Direction::West;
    }
    return stepZ > 0.0F ? Direction::South : Direction::North;
}

MazeCell approachCell(const MazeWorld& world) {
    const Direction side = gateSide(world);
    return {.x = world.exitCell.x + columnStep(side), .z = world.exitCell.z + rowStep(side)};
}

GateScenery gateScenery(const MazeWorld& world) {
    GateScenery scenery;
    scenery.arch = wallModelMatrix(world.gate);

    // Two directions on the ground: out of the exit cell towards the approach, and along
    // the gate. Which way "along" points does not matter: the two bracket lanterns hang
    // the same distance to both sides.
    const Direction side = gateSide(world);
    const glm::vec3 out = directionVector(side);
    const glm::vec3 along = directionVector(leftOf(side));
    const glm::vec3 up{0.0F, 1.0F, 0.0F};
    const glm::vec3 base = world.gate.position;

    scenery.lanterns[0] = placedAt(base + up * GATE_COTE_LANTERN_HEIGHT, GATE_COTE_LANTERN_SCALE);
    const glm::vec3 bracket =
        base + out * GATE_BRACKET_LANTERN_OUT + up * GATE_BRACKET_LANTERN_HEIGHT;
    scenery.lanterns[1] = placedAt(bracket + along * GATE_BRACKET_LANTERN_ALONG, 1.0F);
    scenery.lanterns[2] = placedAt(bracket - along * GATE_BRACKET_LANTERN_ALONG, 1.0F);

    scenery.lightPosition = base + out * GATE_LIGHT_OUT + up * GATE_LIGHT_HEIGHT;

    // The milestone. Seen by somebody who walks towards the gate, "forward" is the
    // opposite of out: so their left is the left of that direction, and behind them is
    // out itself. The first of the three walls that can take a stone gets it.
    const MazeCell approach = approachCell(world);
    const Direction towardsGate = opposite(side);
    const Direction left = leftOf(towardsGate);
    for (const Direction wallSide : {left, opposite(left), side}) {
        if (!world.maze.contains(approach.x, approach.z) ||
            !wallTakesMilestone(world, {.cell = approach, .side = wallSide})) {
            continue;
        }
        // Towards the wall, and towards the end of the cell that is far from the gate.
        // Against the far wall itself the two would be the same direction: the stone
        // then stands in the left corner of that wall.
        glm::vec3 position = cellCenter(approach.x, approach.z) +
                             directionVector(wallSide) * MILESTONE_TO_WALL +
                             (wallSide == side ? directionVector(left) : out) * MILESTONE_TO_MOUTH;
        position.y = world.terrain.heightAt(position.x, position.z);
        scenery.hasMilestone = true;
        scenery.milestoneSide = wallSide;
        scenery.milestone = placedAt(position, 1.0F);
        break;
    }
    return scenery;
}

} // namespace game
