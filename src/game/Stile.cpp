// Stile: where the stile of the start cell stands, what of it blocks the way and where
// feet have worn the grass away in front of it.
#include "game/Stile.hpp"

#include <algorithm>
#include <cmath>

namespace game {

// The wall on the north side of a cell runs along X, and the wall model is not turned on
// such a wall (game::wallModelMatrix). So `along` is the world x and `out`, the side the
// steps are on, is the world +z: south, into the cell. The two functions at the end of
// this file rely on it.
static_assert(STILE_SIDE == Direction::North);

bool carriesStile(const WallSegment& segment, MazeCell start) {
    const WallSegment stile = wallSegmentOn(start.x, start.z, STILE_SIDE);
    return segment.axis == stile.axis && segment.position.x == stile.position.x &&
           segment.position.z == stile.position.z;
}

std::optional<std::size_t> stileWallIndex(std::span<const WallSegment> walls, MazeCell start) {
    const auto found = std::ranges::find_if(
        walls, [start](const WallSegment& segment) { return carriesStile(segment, start); });
    if (found == walls.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(found - walls.begin());
}

scene::Aabb stileBox(const WallSegment& wall) {
    return {.min = wall.position + glm::vec3{STILE_BOX_ALONG_MIN, 0.0F, STILE_BOX_OUT_MIN},
            .max = wall.position +
                   glm::vec3{STILE_BOX_ALONG_MAX, STILE_BOX_HEIGHT, STILE_BOX_OUT_MAX}};
}

bool stileWornGround(const WallSegment& wall, float x, float z) {
    const float along = (x - wall.position.x - STILE_WORN_ALONG) / STILE_WORN_HALF_LENGTH;
    const float out = z - wall.position.z;
    if (std::abs(along) > 1.0F || out < 0.0F) {
        return false;
    }
    // Between the wall and the middle of the oval everything is worn. Beyond the middle
    // the oval decides.
    const float beyond = std::max(out - STILE_WORN_OUT, 0.0F) / STILE_WORN_HALF_DEPTH;
    return along * along + beyond * beyond <= 1.0F;
}

} // namespace game
