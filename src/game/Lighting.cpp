// Lighting of the game: the settings of the moon, the flashlight and the point lights.
// See docs/modules/game/flashlight.md
#include "game/Lighting.hpp"

#include "game/MazeLayout.hpp"

#include <algorithm>
#include <cstddef>

namespace game {

namespace {

// A dead end is closed on three of its four sides.
constexpr int DEAD_END_WALL_COUNT = 3;

} // namespace

SpecularModel specularModelOf(LightingMode mode) {
    return mode == LightingMode::BlinnPhong ? SpecularModel::BlinnPhong : SpecularModel::Phong;
}

bool isDeadEnd(const Maze& maze, int x, int z) {
    int wallCount = 0;
    for (const Direction side : ALL_DIRECTIONS) {
        if (maze.hasWall(x, z, side)) {
            ++wallCount;
        }
    }
    return wallCount == DEAD_END_WALL_COUNT;
}

std::vector<glm::vec3> deadEndLightPositions(const Maze& maze, int skipColumn, int skipRow) {
    // Every dead end, row after row.
    std::vector<glm::vec3> deadEnds;
    for (int z = 0; z < maze.height(); ++z) {
        for (int x = 0; x < maze.width(); ++x) {
            const bool skipped = x == skipColumn && z == skipRow;
            if (!skipped && isDeadEnd(maze, x, z)) {
                deadEnds.push_back(cellCenter(x, z) + glm::vec3{0.0F, POINT_LIGHT_HEIGHT, 0.0F});
            }
        }
    }

    const auto maximum = static_cast<std::size_t>(scene::MAX_POINT_LIGHTS);
    if (deadEnds.size() <= maximum) {
        return deadEnds;
    }

    // Too many: keep maximum of them, spread evenly over the list. With 40 dead ends
    // and 16 lights, i * 40 / 16 gives 0, 2, 5, 7, 10 and so on: whole number division
    // rounds down, and the numbers never repeat because the step is larger than 1.
    std::vector<glm::vec3> chosen;
    chosen.reserve(maximum);
    for (std::size_t i = 0; i < maximum; ++i) {
        chosen.push_back(deadEnds[i * deadEnds.size() / maximum]);
    }
    return chosen;
}

scene::LightSet buildLightSet(const LightingSettings& settings, const glm::vec3& eye,
                              const glm::vec3& viewDirection,
                              std::span<const glm::vec3> pointPositions) {
    scene::LightSet lights;
    lights.ambient = settings.ambient;

    lights.directional = {
        .direction = scene::directionFromAngles(settings.moonYawDegrees, settings.moonPitchDegrees),
        .color = settings.moonColor,
        .intensity = settings.moonIntensity,
    };

    // The flashlight is held at the eye and points where the player looks.
    lights.spot = {
        .position = eye,
        .direction = viewDirection,
        .color = settings.flashlightColor,
        .intensity = settings.flashlightIntensity,
        .attenuation = scene::attenuationForRadius(settings.flashlightRange),
        // A cone cannot be wider inside than outside. The panel keeps the two angles in
        // order, this line keeps them in order whoever sets them.
        .innerConeDegrees =
            std::min(settings.flashlightInnerDegrees, settings.flashlightOuterDegrees),
        .outerConeDegrees = settings.flashlightOuterDegrees,
    };
    lights.spotEnabled = settings.flashlightOn;

    // All point lights share one colour, one intensity and one radius. Only the place
    // differs.
    const scene::Attenuation pointAttenuation = scene::attenuationForRadius(settings.pointRadius);
    const std::size_t pointCount =
        std::min(pointPositions.size(), static_cast<std::size_t>(scene::MAX_POINT_LIGHTS));
    for (std::size_t i = 0; i < pointCount; ++i) {
        lights.points[i] = {
            .position = pointPositions[i],
            .color = settings.pointColor,
            .intensity = settings.pointIntensity,
            .attenuation = pointAttenuation,
        };
    }
    lights.pointCount = static_cast<int>(pointCount);
    return lights;
}

} // namespace game
