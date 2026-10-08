// LightBlock: the bytes of the uniform block the lit shaders read the lights from.
#pragma once

#include "scene/Light.hpp"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace scene {

// The lights reach the shaders through one uniform block, declared in
// assets/shaders/common/lighting.glsl:
//
//     layout(std140) uniform LightBlock { ... };
//
// "std140" is a layout rule of the GLSL standard: it fixes at which byte every member of
// the block starts, the same on every driver. The structs below are the C++ picture of
// that block: filled here, copied to a gfx::UniformBuffer byte for byte and read by the
// shader. So they must put every member at exactly the byte std140 puts it.
//
// The std140 rules that matter here:
//   - a float or an int is 4 bytes and starts at a multiple of 4,
//   - a vec4 is 16 bytes and starts at a multiple of 16,
//   - a vec3 is 12 bytes but ALSO starts at a multiple of 16, which leaves holes that are
//     easy to get wrong,
//   - a struct and every element of an array start at a multiple of 16, and their size is
//     rounded up to a multiple of 16.
//
// To keep the layout obvious, the block uses vec4 for everything except the number of
// point lights. A position or a colour needs only three of the four floats, and the
// fourth carries a small extra value (an intensity, a switch) or nothing. One vec4 is
// one glm::vec4: 16 bytes after 16 bytes, without holes.

/// One element of the array uPoints of the block: 3 vec4, 48 bytes. 48 is a multiple of
/// 16, so the elements follow each other without padding.
struct PointLightData {
    glm::vec4 position;    ///< x, y, z: position in world space. w: not used.
    glm::vec4 color;       ///< x, y, z: red, green, blue. w: intensity.
    glm::vec4 attenuation; ///< x: constant, y: linear, z: quadratic term. w: not used.
};

/// The whole block, member for member in the order of the GLSL declaration. The comments
/// give the name in the shader and the offset in bytes.
struct LightBlockData {
    glm::vec4 cameraPosition;       ///< uCameraPosition, 0. x, y, z: the eye in world space.
    glm::vec4 ambient;              ///< uAmbient, 16. x, y, z: red, green, blue.
    glm::vec4 directionalDirection; ///< uDirectionalDirection, 32. x, y, z: the way the light
                                    ///< travels, length 1.
    glm::vec4 directionalColor;     ///< uDirectionalColor, 48. x, y, z: colour. w: intensity.
    glm::vec4 spotPosition;         ///< uSpotPosition, 64. x, y, z: position in world space.
    glm::vec4 spotDirection;        ///< uSpotDirection, 80. x, y, z: axis of the cone, length 1.
    glm::vec4 spotColor;            ///< uSpotColor, 96. x, y, z: colour. w: intensity.
    glm::vec4 spotAttenuation;      ///< uSpotAttenuation, 112. x, y, z: the three terms.
    glm::vec4 spotCone;             ///< uSpotCone, 128. x: cosine of the inner angle, y: of the
                                    ///< outer angle, z: 1 when the light is on, 0 when off.
    std::int32_t pointCount;        ///< uPointCount, 144. An int is 4 bytes.
    /// Not in the shader. The array below starts at a multiple of 16, so std140 skips the
    /// 12 bytes after uPointCount. These three ints stand in that hole, which makes the
    /// hole visible here and keeps it filled with zeros.
    std::array<std::int32_t, 3> padding;
    std::array<PointLightData, MAX_POINT_LIGHTS> points; ///< uPoints, 160. 16 times 48 bytes.
};

// The asserts below fail the build when a member is added, removed or moved without the
// same change in common/lighting.glsl in mind. They are checked by the compiler, so
// a wrong layout never reaches a running program.

// offsetof is only defined for "standard layout" types (no virtual functions, all
// members with the same access), which both structs are.
static_assert(std::is_standard_layout_v<PointLightData>);
static_assert(std::is_standard_layout_v<LightBlockData>);

// A glm::vec4 must be four floats and nothing else.
static_assert(sizeof(glm::vec4) == 16);

static_assert(sizeof(PointLightData) == 48);
static_assert(offsetof(PointLightData, position) == 0);
static_assert(offsetof(PointLightData, color) == 16);
static_assert(offsetof(PointLightData, attenuation) == 32);

static_assert(offsetof(LightBlockData, cameraPosition) == 0);
static_assert(offsetof(LightBlockData, ambient) == 16);
static_assert(offsetof(LightBlockData, directionalDirection) == 32);
static_assert(offsetof(LightBlockData, directionalColor) == 48);
static_assert(offsetof(LightBlockData, spotPosition) == 64);
static_assert(offsetof(LightBlockData, spotDirection) == 80);
static_assert(offsetof(LightBlockData, spotColor) == 96);
static_assert(offsetof(LightBlockData, spotAttenuation) == 112);
static_assert(offsetof(LightBlockData, spotCone) == 128);
static_assert(offsetof(LightBlockData, pointCount) == 144);
static_assert(offsetof(LightBlockData, padding) == 148);
static_assert(offsetof(LightBlockData, points) == 160);
// 160 bytes before the array plus 16 lights of 48 bytes.
static_assert(sizeof(LightBlockData) == 160 + MAX_POINT_LIGHTS * 48);
static_assert(sizeof(LightBlockData) == 928);

/// Fills the block from a LightSet and the position the scene is seen from (the eye of
/// the camera in world space: the shiny highlight depends on where the viewer is).
///
/// Directions are brought to length 1 here, so the shader does not have to. Cone angles
/// become cosines (coneCosines). pointCount is limited to 0 to MAX_POINT_LIGHTS. Every
/// float that the shader does not read, and every point light past pointCount, is zero.
LightBlockData packLightBlock(const LightSet& lights, const glm::vec3& cameraPosition);

} // namespace scene
