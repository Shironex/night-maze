// Lighting of the game: the settings of the moon, the flashlight and the point lights.
// See docs/modules/game/flashlight.md
#pragma once

#include "game/Maze.hpp"
#include "scene/Light.hpp"

#include <glm/glm.hpp>

#include <span>
#include <vector>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The class that sends the lights to the graphics card is game::LightRig.

/// How the maze is shaded. The debug UI shows the entries in this order, so the number
/// of an entry in its list is the value of the enum.
enum class LightingMode {
    Unlit = 0,  ///< no lighting: the texture as it is (the textured program)
    Gouraud,    ///< lighting computed for every vertex and blended across the triangle
    Phong,      ///< lighting computed for every fragment, highlight from the reflected ray
    BlinnPhong, ///< lighting computed for every fragment, highlight from the halfway vector
};

/// The value of the uniform uSpecularModel in common/lighting.glsl: which formula the
/// shiny highlight is computed with. The numbers are the ones the shader compares with.
enum class SpecularModel {
    Phong = 0,      ///< angle between the reflected light ray and the direction to the eye
    BlinnPhong = 1, ///< angle between the normal and the halfway vector
};

/// The highlight formula a lighting mode uses. Gouraud uses the classic Phong formula,
/// so that the modes Gouraud and Phong differ in one thing only: where the lighting is
/// computed (per vertex or per fragment).
SpecularModel specularModelOf(LightingMode mode);

/// Everything about the lighting that can be changed while the game runs. The debug UI
/// edits these fields, and the game builds the lights of a frame from them
/// (buildLightSet). The values here are the defaults of the night scene.
struct LightingSettings {
    /// How the maze is shaded.
    LightingMode mode = LightingMode::BlinnPhong;

    /// Light that reaches every surface: low, so that corners no light shines into are
    /// dark but not black. A cold blue, like the night sky.
    glm::vec3 ambient{0.035F, 0.045F, 0.075F};

    /// The moon, a directional light. The two angles say which way its light TRAVELS,
    /// with the conventions of scene::directionFromAngles: yaw like a compass (0 towards
    /// -Z, 90 towards +X), pitch below 0 means downwards. The yaw is deliberately not
    /// a multiple of 45 degrees: the walls face four directions, and this way each of
    /// them gets a different share of the light. There are no shadows before M7, so the
    /// moon also lights walls and floor that stand in the shade of another wall.
    float moonYawDegrees = 25.0F;
    float moonPitchDegrees = -50.0F;
    /// A cool, dim blue-white.
    glm::vec3 moonColor{0.55F, 0.65F, 1.0F};
    float moonIntensity = 0.3F;

    /// The flashlight, a spot light at the eye of the player. Key F switches it.
    bool flashlightOn = true;
    /// A warm white.
    glm::vec3 flashlightColor{1.0F, 0.9F, 0.72F};
    float flashlightIntensity = 1.6F;
    /// Half angles of the cone in degrees, see scene::SpotLight.
    float flashlightInnerDegrees = 13.0F;
    float flashlightOuterDegrees = 21.0F;
    /// How far the flashlight reaches, in metres (scene::attenuationForRadius).
    float flashlightRange = 16.0F;

    /// The point lights in the dead ends of the maze. They all share these settings.
    /// A cyan-teal, the colour of the crystals that will take their place.
    glm::vec3 pointColor{0.2F, 0.9F, 0.8F};
    float pointIntensity = 2.0F;
    /// How far one of them reaches, in metres: one and a half cells.
    float pointRadius = 3.0F;

    /// The shiny highlight of the stone. Strength: how bright it is compared with the
    /// light that makes it. Stone is rough, so it is weak.
    float specularStrength = 0.25F;
    /// Shininess: the exponent of the highlight formula. A larger number makes the
    /// highlight smaller and sharper. 32 is a common middle value.
    float shininess = 32.0F;

    /// Normal mapping: the normal of every fragment is read from the normal map of the
    /// material instead of being taken from the mesh, which gives the flat walls and the
    /// floor joints and bumps under the lights. See usesNormalMap for where it applies.
    bool normalMapping = true;
};

/// True when the normals come from the normal maps with these settings: normal mapping
/// is switched on and the lighting mode is not Gouraud.
///
/// A normal map holds a normal per texel, so it needs lighting per fragment (Phong and
/// Blinn-Phong). Gouraud computes the light at the vertices only and cannot use it. The
/// mode Unlit has no lighting at all, but the debug view "Normals as colour" shows the
/// normals of the maps in it, so the answer is true there.
bool usesNormalMap(const LightingSettings& settings);

/// How high above the floor a point light of the maze hangs, in metres.
constexpr float POINT_LIGHT_HEIGHT = 1.4F;

/// True when the cell in column x and row z is a dead end: it has a wall on exactly
/// three of its four sides, so there is one way in and no way on.
/// Throws std::out_of_range when the cell is not in the maze.
bool isDeadEnd(const Maze& maze, int x, int z);

/// Where the point lights of a maze hang: POINT_LIGHT_HEIGHT above the centre of every
/// dead end, except the cell (skipColumn, skipRow), which is the start cell.
///
/// The order is fixed: row after row, cell after cell. At most scene::MAX_POINT_LIGHTS
/// positions are returned. A maze with more dead ends than that keeps an evenly spread
/// choice of them (number i of the result is dead end number i * count / maximum), so
/// the lights are not all in the first rows. Nothing here is random: the same maze
/// always gets the same lights.
std::vector<glm::vec3> deadEndLightPositions(const Maze& maze, int skipColumn, int skipRow);

/// The lights of one frame.
///
/// eye and viewDirection are where the camera stands and where it looks in this frame:
/// the flashlight is put exactly there, so its cone stays in the middle of the picture.
/// Pass the same eye the view matrix is built from (the blend of two fixed steps), not
/// the position of the last step, or the cone would trail behind the picture.
///
/// pointPositions are the places of the point lights (MazeWorld::pointLightPositions).
/// Positions past scene::MAX_POINT_LIGHTS are ignored.
scene::LightSet buildLightSet(const LightingSettings& settings, const glm::vec3& eye,
                              const glm::vec3& viewDirection,
                              std::span<const glm::vec3> pointPositions);

} // namespace game
