// LightSpace: the scene as a light sees it, the two matrices a shadow map is drawn and read with.
#pragma once

#include <glm/glm.hpp>

namespace scene {

struct Aabb;

// Plain math without OpenGL, so tests can use it. A shadow map is a picture of the scene
// taken from the light: every texel stores how far the nearest surface is. To take that
// picture the light needs what a camera has, a view matrix and a projection matrix.
// Together they are called the light space.

/// Free room around the box of a directional light on all six sides, in metres. A caster
/// that lies exactly on a side of the box (the lowest ground, the top of a pillar) is
/// then safely inside and not cut off by a rounding error.
constexpr float LIGHT_BOX_MARGIN = 0.5F;

/// Above this value of |y| a light direction (of length 1) counts as vertical: about 2.5
/// degrees away from straight down or straight up. See directionalLightSpace and
/// spotLightSpace.
constexpr float VERTICAL_DIRECTION_LIMIT = 0.999F;

/// The smallest and the largest full opening angle of the shadow map of a spot light, in
/// degrees. A perspective projection needs an angle above 0 and below 180: at 180 the
/// picture would be endlessly wide.
constexpr float MIN_SPOT_FIELD_OF_VIEW_DEGREES = 1.0F;
constexpr float MAX_SPOT_FIELD_OF_VIEW_DEGREES = 170.0F;

/// How much farther than the outer cone of a spot light its shadow map looks, in degrees,
/// to every side. The light ends at the outer cone, and a square map that is exactly as
/// wide would touch that circle in the middle of its four sides: the PCF kernel of
/// a fragment there would reach over the edge of the map. With this margin the whole
/// cone and the texels around it are inside. A map of 1024 texels over 46 degrees has
/// about 22 texels per degree, so 2 degrees are more than 40 texels.
constexpr float SPOT_CONE_MARGIN_DEGREES = 2.0F;

/// The near plane of the shadow map of a spot light, in metres from the light. Nothing
/// nearer to the light is drawn into the map, so nothing nearer casts a shadow. It
/// cannot be 0 (a perspective projection divides by the distance), and the smaller it
/// is, the less exact the stored depths are far away. With 5 cm a depth of 24 bits
/// still tells two surfaces apart that are 16 m away and less than a millimetre from
/// each other.
constexpr float SPOT_NEAR_PLANE = 0.05F;

/// The two kinds of projection a light space can have.
enum class LightProjection {
    /// A box: parallel rays, and a stored depth that grows evenly with the distance.
    /// The moon.
    Orthographic,
    /// A pyramid with its tip at the light: rays that spread out, and a stored depth
    /// that changes fast near the light and hardly at all far from it. The flashlight.
    Perspective,
};

/// The view and the projection of a light.
struct LightSpace {
    /// World space to the space of the light: the light looks along -Z, like a camera.
    glm::mat4 view{1.0F};
    /// The space of the light to clip space.
    glm::mat4 projection{1.0F};
    /// The size of what the shadow map covers, in metres.
    /// Orthographic (a directional light): x and y are the width and the height of the
    /// area the map covers, z is the distance from its near plane to its far plane.
    /// Perspective (a spot light): x and y are the width and the height of the area the
    /// map covers AT THE FAR PLANE (nearer to the light it covers less, in proportion
    /// to the distance), z is again the distance from the near plane to the far plane.
    glm::vec3 extent{0.0F};

    /// Which of the two projections this is. The code that reads the map has to know:
    /// a bias in metres, the size of a texel and the preview picture are computed
    /// differently for a depth that grows evenly and for one that does not.
    LightProjection kind = LightProjection::Orthographic;
    /// Perspective only: where the light stands in world space, and its near and its
    /// far plane as distances from it in metres. A directional light has no position,
    /// and its planes are not needed (extent.z is its whole depth range), so the three
    /// stay at 0 for it.
    glm::vec3 position{0.0F};
    float nearPlane = 0.0F;
    float farPlane = 0.0F;

    /// World space to the clip space of the light in one matrix. A vertex shader
    /// applies the view first and the projection second, so the projection stands on
    /// the left.
    glm::mat4 matrix() const { return projection * view; }
};

/// The light space of a directional light (the moon): an ORTHOGRAPHIC box, because the
/// rays of such a light are parallel and nothing gets smaller with distance.
///
/// lightDirection is the way the light TRAVELS, in world space, of any length. The box
/// is turned so that the light looks along it, and made exactly as large as it has to
/// be to hold bounds (plus LIGHT_BOX_MARGIN): the eight corners of bounds are brought
/// into the space of the light, and the smallest and the largest x, y and z among them
/// are the sides of the box. A box that fits wastes no texel of the shadow map on empty
/// space.
///
/// The result depends on bounds and on the direction only, not on any camera. The shadow
/// map therefore covers the same ground in every frame, and the shadows do not shimmer
/// when the player moves.
///
/// A view matrix needs an "up" direction that is not parallel to the viewing direction.
/// World up (+Y) is used, except for a light that shines (almost) straight down or up
/// (VERTICAL_DIRECTION_LIMIT): then -Z is used. A direction of length 0 is replaced by
/// straight down.
LightSpace directionalLightSpace(const Aabb& bounds, const glm::vec3& lightDirection);

/// The light space of a spot light (the flashlight): a PERSPECTIVE projection, because
/// the rays of such a light start in one point and spread out. It is the picture
/// a camera would take that stands where the light is and looks along its cone.
///
/// position is where the light stands and direction the axis of its cone (the way the
/// light travels), in world space, of any length. outerConeDegrees is the angle from
/// the axis to the side of the cone (scene::SpotLight). The map is a square that looks
/// that far to every side plus SPOT_CONE_MARGIN_DEGREES, so its full opening angle is
/// twice that sum, kept inside MIN_SPOT_FIELD_OF_VIEW_DEGREES to
/// MAX_SPOT_FIELD_OF_VIEW_DEGREES. range is how far the light reaches in metres: it is
/// the far plane. The near plane is SPOT_NEAR_PLANE. A range that is not behind the
/// near plane is moved just behind it.
///
/// The "up" direction is chosen as for a directional light: world up, or -Z for a cone
/// that points (almost) straight up or down. A direction of length 0 is replaced by
/// straight down.
///
/// Unlike the box of a directional light, this pyramid moves with its light: the map is
/// a new picture in every frame in which the light has moved or turned.
LightSpace spotLightSpace(const glm::vec3& position, const glm::vec3& direction,
                          float outerConeDegrees, float range);

/// Where a point of the world lands in a shadow map: x and y are the texture coordinate
/// (0 to 1 inside the map) and z is the depth the map stores for a surface at that point
/// (0 at the near plane of the light, 1 at its far plane). lightSpaceMatrix is
/// LightSpace::matrix(). The same steps are in common/shadows.glsl: the matrix, the
/// division by w, and from the range -1..1 to the range 0..1.
///
/// With a perspective light space the result has a meaning only for a point IN FRONT of
/// the light: behind it w is negative, and the division would mirror the point into the
/// map. The shader tests for that before it divides.
glm::vec3 shadowMapCoordinates(const glm::mat4& lightSpaceMatrix, const glm::vec3& worldPosition);

} // namespace scene
