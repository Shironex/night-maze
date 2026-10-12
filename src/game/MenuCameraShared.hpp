// What MenuCamera.cpp and MenuCameraPath.cpp share: the numbers both use and the place of
// a distance on the path.
#pragma once

#include "game/MenuCamera.hpp"

#include <glm/gtc/constants.hpp>

#include <cstddef>

namespace game {

// The windows and reaches above are never longer than this share of the whole loop. It
// only matters for a very short loop (a maze of one or two cells), where a window of
// their full length would reach around the whole loop.
constexpr float MAX_WINDOW_SHARE = 0.25F;

// A length below this counts as no length, in metres: nothing is divided by it.
constexpr float MIN_LENGTH = 0.0001F;

// One full turn in degrees and in radians.
constexpr float FULL_TURN_DEGREES = 360.0F;
constexpr float FULL_TURN = glm::two_pi<float>();

// Where a distance along the path falls: on the piece that starts at point before and
// ends at point next (the first point again for the piece that closes the loop), share
// of the way from the one to the other (0 to 1).
struct PathPlace {
    std::size_t before = 0;
    std::size_t next = 0;
    bool closing = false;
    float share = 0.0F;
};

/// Finds the place of a distance on a path that has points and a length.
PathPlace placeOnPath(const MenuCameraPath& path, float distance);

} // namespace game
