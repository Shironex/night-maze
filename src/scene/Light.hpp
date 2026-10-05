// Lights: a directional light, point lights and a spot light as plain data, plus their math.
// See docs/modules/scene/lights.md
#pragma once

#include <glm/glm.hpp>

#include <array>

namespace scene {

// Everything here is plain data and math: no OpenGL, so tests can use it. The shaders
// (assets/shaders/common/lighting.glsl) do the same computations for every fragment or
// vertex. The helpers below exist to prepare the data and to check the formulas.

/// Largest number of point lights in a LightSet. The array of point lights in the
/// shader (MAX_POINT_LIGHTS in common/lighting.glsl) has the same length.
constexpr int MAX_POINT_LIGHTS = 16;

/// How a light gets weaker with distance. The brightness at distance d (in metres) is
/// multiplied by 1 / (constant + linear * d + quadratic * d * d).
struct Attenuation {
    /// Part that does not depend on the distance. With 1 the light has its full
    /// brightness at distance 0 and can never be brighter than that.
    float constant = 1.0F;
    /// Part that grows with the distance.
    float linear = 0.0F;
    /// Part that grows with the square of the distance: the physical one, light spreads
    /// over the surface of a sphere, and that surface grows with the square of its radius.
    float quadratic = 0.0F;
};

/// A light that is so far away that its rays are parallel: the moon. It has a direction
/// but no position, and it does not get weaker with distance.
struct DirectionalLight {
    /// The direction the light TRAVELS in, from the light towards the scene, in world
    /// space. (0, -1, 0) shines straight down. It does not have to be of length 1.
    glm::vec3 direction{0.0F, -1.0F, 0.0F};
    /// Red, green, blue, each from 0 to 1.
    glm::vec3 color{1.0F};
    /// The colour is multiplied by this. 0 switches the light off.
    float intensity = 1.0F;
};

/// A light that shines from one point in all directions and gets weaker with distance.
struct PointLight {
    /// Position in world space.
    glm::vec3 position{0.0F};
    glm::vec3 color{1.0F};
    float intensity = 1.0F;
    Attenuation attenuation;
};

/// A point light that only shines into a cone: the flashlight.
struct SpotLight {
    /// Position of the tip of the cone in world space.
    glm::vec3 position{0.0F};
    /// The direction the cone points in (its axis), in world space. It does not have to
    /// be of length 1.
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
    glm::vec3 color{1.0F};
    float intensity = 1.0F;
    Attenuation attenuation;
    /// Both angles are measured from the axis of the cone to its side, in degrees (half
    /// of the full opening angle). Inside the inner cone the light has its full
    /// brightness, outside the outer cone there is none, and between the two it fades.
    /// The inner angle must not be larger than the outer one.
    float innerConeDegrees = 12.0F;
    float outerConeDegrees = 18.0F;
};

/// All lights of a scene: what the lit shaders work with in one frame.
struct LightSet {
    /// Light that reaches every surface from every side, so that a place no light shines
    /// on is dark but not black. Red, green, blue.
    glm::vec3 ambient{0.0F};

    DirectionalLight directional;

    /// Only the first pointCount elements are lights, the rest is ignored.
    std::array<PointLight, MAX_POINT_LIGHTS> points;
    int pointCount = 0;

    SpotLight spot;
    /// False switches the spot light off without changing its settings.
    bool spotEnabled = true;
};

/// The share of its brightness a light made by attenuationForRadius still has at its
/// radius: 1 / 20, which is 5 %.
constexpr float BRIGHTNESS_AT_RADIUS = 0.05F;

/// Attenuation terms for a light that should reach about radius metres:
///
///     constant = 1,  linear = 2 / radius,  quadratic = 17 / (radius * radius)
///
/// At distance d = radius the divisor is 1 + 2 + 17 = 20, so the light is down to
/// BRIGHTNESS_AT_RADIUS (5 %) of its brightness there, whatever the radius is. The curve
/// has the same shape for every radius, only stretched. The light never reaches exactly
/// zero: 1 / (something) is always above 0, so beyond the radius it is still there, only
/// too weak to be seen. A radius that is not positive gives a light without attenuation.
Attenuation attenuationForRadius(float radius);

/// The factor a light with these terms is multiplied by at the given distance in metres:
/// 1 / (constant + linear * d + quadratic * d * d). The same formula is in the shader.
float attenuationFactor(const Attenuation& attenuation, float distance);

/// The cosines of the two cone angles of a spot light, which is what the shader compares
/// with (see spotFactor).
struct ConeCosines {
    /// Cosine of the inner angle: the larger of the two numbers, because the cosine
    /// falls as the angle grows.
    float inner = 1.0F;
    /// Cosine of the outer angle.
    float outer = 0.0F;
};

/// Smallest difference between the two cosines. The fade between the cones divides by
/// that difference, so it must never be 0.
constexpr float MIN_CONE_COSINE_GAP = 0.001F;

/// Turns the two cone angles (degrees, measured from the axis) into their cosines. When
/// the inner angle is not smaller than the outer one (a cone with a hard edge, or wrong
/// input), the inner cosine is set MIN_CONE_COSINE_GAP above the outer one, so the result
/// always has inner > outer.
ConeCosines coneCosines(float innerDegrees, float outerDegrees);

/// How much of a spot light reaches a point, from 0 to 1. cosAngle is the cosine of the
/// angle between the axis of the cone and the line from the light to the point: 1 inside
/// the inner cone, 0 outside the outer cone, and a straight ramp in between (the soft
/// edge). The same formula is in the shader.
float spotFactor(const ConeCosines& cone, float cosAngle);

/// A unit vector from two angles in degrees, with the conventions of scene::Camera: yaw
/// 0 points along -Z, 90 along +X, 180 along +Z, 270 along -X. Pitch 0 is level, positive
/// points up, negative down. Used for the direction of the moon light, which is easier
/// to set as two angles than as three numbers.
glm::vec3 directionFromAngles(float yawDegrees, float pitchDegrees);

} // namespace scene
