// Shadows: the settings of a shadow map, and the math around it that needs no OpenGL.
#pragma once

#include "scene/Collider.hpp"

namespace scene {
struct LightSpace;
} // namespace scene

namespace game {

class Terrain;

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The class that owns the depth texture is game::ShadowMap, and the shaders
// that read it include assets/shaders/common/shadows.glsl.
//
// Shadow mapping in two steps. First the scene is drawn from the light into a depth
// texture, the shadow map: every texel then holds the distance from the light to the
// nearest surface. Later, for every fragment of the real picture, the distance from the
// light to that fragment is compared with the texel it falls on. A fragment that is
// farther away than the stored distance has something between it and the light: it is
// in shadow.

/// The sizes a shadow map can have: it is a square of this many texels in each
/// direction. The debug UI shows the entries in this order, so the number of an entry
/// in its list is the value of the enum.
enum class ShadowResolution {
    Low = 0,  ///< SHADOW_MAP_SIZE_LOW
    High = 1, ///< SHADOW_MAP_SIZE_HIGH
};

/// The two sizes in texels. Four times the texels cost four times the memory and the
/// fill time: 2048 x 2048 depths of 24 bits are about 12 MB, or 16 MB where the card
/// keeps each depth in four bytes.
constexpr int SHADOW_MAP_SIZE_LOW = 1024;
constexpr int SHADOW_MAP_SIZE_HIGH = 2048;

/// The side of a shadow map of the given resolution, in texels.
int shadowMapSize(ShadowResolution resolution);

/// PCF (percentage closer filtering) compares not one texel but a square of them around
/// the fragment and takes the average of the answers, which turns the hard, stepped
/// edge of a shadow into a soft one. The radius is how many texels the square reaches
/// to each side of the middle one: 1 is a kernel of 3 x 3 comparisons, 2 of 5 x 5, 3 of
/// 7 x 7. MAX_PCF_RADIUS is also a constant in common/shadows.glsl.
constexpr int MIN_PCF_RADIUS = 1;
constexpr int MAX_PCF_RADIUS = 3;
constexpr int DEFAULT_PCF_RADIUS = 1;

/// The side of the PCF kernel for a radius: 2 * radius + 1.
int pcfKernelSide(int radius);

/// What can be changed about one shadow map while the game runs. The debug UI edits the
/// fields. Every light with a shadow map has one of these: the moon and the flashlight.
/// The defaults below are the ones of the moon. The flashlight starts with
/// flashlightShadowDefaults.
struct ShadowSettings {
    /// False: the shadow map is not drawn and nothing is in the shadow of this light.
    /// With the shadows of every light switched off the picture is exactly the one of
    /// the game without shadows.
    bool enabled = true;

    /// The size of the shadow map.
    ShadowResolution resolution = ShadowResolution::High;

    /// The bias, in metres: how much nearer to the light a surface is taken to be when
    /// it is compared with the shadow map (shadowBias). It is there because a texel of
    /// the map is not a point: it covers a small patch of the surface and stores one
    /// depth for all of it. On a surface that is tilted against the light, half of the
    /// patch is farther from the light than that one depth, and without a bias that
    /// half would shade itself, in stripes: "shadow acne".
    ///
    /// The constant part is always added. The slope part is added in full on a surface
    /// the light only grazes and not at all on one that faces the light, because the
    /// depth inside one texel differs the more the more the surface is tilted. Too much
    /// bias has a price of its own: the shadow starts a little behind the thing that
    /// casts it and seems to come loose from it ("peter panning"). The walls are 0.2 m
    /// thick, so that shows only for a bias of about that size.
    float constantBias = 0.02F;
    float slopeBias = 0.12F;

    /// True: the graphics card compares the four texels around the fragment and blends
    /// the four answers (a comparison sampler with a linear filter, "hardware PCF").
    /// False: one texel, one answer, and the texels show as steps.
    bool hardwareFilter = true;

    /// The PCF kernel on top of that: the switch and the radius (MIN_PCF_RADIUS to
    /// MAX_PCF_RADIUS).
    bool pcf = true;
    int pcfRadius = DEFAULT_PCF_RADIUS;

    /// How dark a shadow is: the share of the light that a shadow takes away. 1 leaves
    /// nothing of the light in a shadow, 0 makes the shadows invisible. Only the light
    /// that owns the map is affected, never the ambient light or another light.
    float strength = 1.0F;

    /// Whether the preview picture of the shadow map is drawn in this frame. It costs
    /// a small pass, so it is made only while the Shadows panel of the debug UI shows
    /// the tab of this light: the debug UI sets this field every frame.
    bool preview = false;
};

/// The bias the shadow map of the flashlight starts with, in metres (the two bias fields
/// of ShadowSettings). The map of the flashlight is a pyramid, so its texels are not of
/// one size: they grow with the distance from the light. At 1024 texels over an opening
/// angle of 46 degrees a texel is 0.08 cm wide for every metre of distance: 0.3 cm on
/// a wall 4 m away, where a texel of the moon has 3.2 cm.
///
/// The constant part is therefore smaller than the one of the moon: on a surface that
/// faces the flashlight the depth inside a texel differs by millimetres.
///
/// The slope part is what the ground needs. The flashlight is held about 1.45 m above
/// it, so its light grazes the ground more and more the farther ahead it falls. On the
/// ground 10 m ahead, one texel to the side is 5.8 cm nearer to the light or farther
/// from it, and the PCF kernel of 3 x 3 with the hardware filter reaches 2 texels:
/// 11.6 cm. The bias there is 0.01 + 0.13 * (1 - 0.14) = 12.1 cm. Farther ahead the
/// bias is too small, but hardly any light arrives there. The largest bias is 0.14 m,
/// as for the moon: less than the 0.2 m of a wall. A test checks these numbers.
constexpr float FLASHLIGHT_SHADOW_CONSTANT_BIAS = 0.01F;
constexpr float FLASHLIGHT_SHADOW_SLOPE_BIAS = 0.13F;

/// The settings the shadow map of the flashlight starts with: the small map and the bias
/// above, everything else as for the moon. The small map is enough, because the pyramid
/// of the flashlight covers far less than the box of the moon: even at the end of its
/// default reach of 10 m a texel is 0.8 cm, a quarter of the 3.2 cm of the moon.
ShadowSettings flashlightShadowDefaults();

/// The box that holds everything that can cast a shadow of the moon: the whole land of
/// the terrain, from its lowest ground up to PILLAR_HEIGHT above its highest ground. The
/// walls, the pillars, the gate and the crystals all stand on the ground and are no
/// higher than a pillar, so they are inside. The moon fits its shadow map to this box
/// (scene::directionalLightSpace).
scene::Aabb shadowCasterBounds(const Terrain& terrain);

/// The bias of a surface in metres:
///
///     bias = constantBias + slopeBias * (1 - facing)
///
/// facing is the cosine of the angle between the normal of the surface and the
/// direction to the light: 1 for a surface that faces the light, 0 for one the light
/// only grazes. Values outside 0 to 1 are brought into that range, so a surface that
/// faces away gets the full slope part. The same formula is in common/shadows.glsl.
float shadowBias(float constantBias, float slopeBias, float facing);

/// A bias in metres as a difference of the depths a shadow map stores. The map of
/// a directional light stores depths from 0 to 1 that grow evenly over depthRange
/// metres (the z of scene::LightSpace::extent), so the bias is divided by that range.
/// A range that is not positive gives 0.
float biasInDepthUnits(float biasMetres, float depthRange);

/// A bias in metres as the number the shaders want for the shadow map of lightSpace (the
/// two bias uniforms of a map in common/shadows.glsl). It depends on the projection:
///
/// Orthographic (the moon): the stored depth grows evenly, so a metre is always the same
/// share of the stored range, and the bias is handed over as that share
/// (biasInDepthUnits with the depth range of the box). The shader subtracts it from the
/// depth of the fragment.
///
/// Perspective (the flashlight): the stored depth does not grow evenly. A metre near
/// the light is a large step of it and a metre far away a tiny one, so no number of
/// depth units is "one metre". The bias stays IN METRES, and the shader moves the
/// fragment that far towards the light before it projects it into the map.
float biasForShader(float biasMetres, const scene::LightSpace& lightSpace);

/// The size of one texel of a shadow map on a surface that faces the light, in metres:
/// the larger of the width and the height of the area the map covers, divided by the
/// number of texels along a side. Smaller is sharper. A size that is not positive gives
/// 0. For a perspective light space this is the size at its far plane, where the texels
/// are largest: see shadowTexelSizeAt.
float shadowTexelSize(const scene::LightSpace& lightSpace, int mapSize);

/// The size of one texel of a shadow map at the given distance from the light, in
/// metres, on a surface that faces the light. In the box of a directional light a texel
/// has the same size everywhere and the distance does not matter. In the pyramid of
/// a spot light the area the map covers grows in proportion to the distance from the
/// light, and its texels with it: at half of the far plane they are half as large as at
/// the far plane. A distance below 0 counts as 0.
float shadowTexelSizeAt(const scene::LightSpace& lightSpace, int mapSize, float distance);

/// The radius the shaders use (the PCF radius uniform of a map in common/shadows.glsl):
/// the radius of the settings brought into MIN_PCF_RADIUS to MAX_PCF_RADIUS, or 0 when
/// the PCF kernel is switched off. With 0 a shader makes one comparison.
int pcfRadiusInUse(const ShadowSettings& settings);

} // namespace game
