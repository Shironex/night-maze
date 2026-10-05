// LightSpace: the scene as a light sees it, the two matrices a shadow map is drawn and read with.
// See docs/modules/renderer/shadows.md
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
/// degrees away from straight down or straight up. See directionalLightSpace.
constexpr float VERTICAL_DIRECTION_LIMIT = 0.999F;

/// The view and the projection of a light.
struct LightSpace {
    /// World space to the space of the light: the light looks along -Z, like a camera.
    glm::mat4 view{1.0F};
    /// The space of the light to clip space.
    glm::mat4 projection{1.0F};
    /// The size of the orthographic box of a directional light in metres: x and y are
    /// the width and the height of the area the shadow map covers, z is the distance
    /// from its near plane to its far plane.
    glm::vec3 extent{0.0F};

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

/// Where a point of the world lands in a shadow map: x and y are the texture coordinate
/// (0 to 1 inside the map) and z is the depth the map stores for a surface at that point
/// (0 at the near plane of the light, 1 at its far plane). lightSpaceMatrix is
/// LightSpace::matrix(). The same steps are in common/shadows.glsl: the matrix, the
/// division by w, and from the range -1..1 to the range 0..1.
glm::vec3 shadowMapCoordinates(const glm::mat4& lightSpaceMatrix, const glm::vec3& worldPosition);

} // namespace scene
