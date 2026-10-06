// Names of the uniform variables of the shaders in assets/shaders, in one place.
// See docs/modules/gfx/uniforms.md
#pragma once

#include <glad/gl.h>

namespace game {

// A uniform is found by its name (gfx::Shader::setMat4 and the other setters), so each
// string below must be spelled exactly like the "uniform" line of the shader file. A
// name with a typo is not an error: OpenGL silently ignores it. Keeping the names here,
// once, means that the classes that draw cannot disagree about them.

/// The three matrices. Every vertex shader (textured, color, lit, gouraud) declares
/// them under the same names. skybox.vert has the view and the projection only: the sky
/// is not placed anywhere in the world. The grass has the same two, in grass.geom: its
/// points are already in world space. shadow_depth.vert has all three, and there the
/// view and the projection are the ones of a light (scene::LightSpace).
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";

/// textured.frag, lit.frag and gouraud.frag: the sampler (it holds the number of
/// a texture unit) and the colour the texture is multiplied by.
constexpr const char* TEXTURE_UNIFORM = "uTexture";
constexpr const char* TINT_UNIFORM = "uTint";

/// textured.frag, lit.frag and gouraud.frag: the light a surface gives off by itself,
/// as a linear colour. Black for everything except the crystals.
constexpr const char* EMISSIVE_UNIFORM = "uEmissive";

/// common/normal_map.glsl, so lit.frag and textured.frag: the sampler of the normal map
/// (the number of a second texture unit) and the switch of normal mapping (1 on, 0 off).
constexpr const char* NORMAL_MAP_UNIFORM = "uNormalMap";
constexpr const char* NORMAL_MAP_ENABLED_UNIFORM = "uNormalMapEnabled";

/// textured.frag, skybox.frag and grass.frag: what to show (a value of game::ViewMode).
constexpr const char* VIEW_MODE_UNIFORM = "uViewMode";

/// lit.vert and gouraud.vert: the matrix that takes normals to world space
/// (scene::normalMatrix).
constexpr const char* NORMAL_MATRIX_UNIFORM = "uNormalMatrix";

/// common/lighting.glsl, so lit.frag, gouraud.vert and grass.frag: the highlight formula
/// (a value of game::SpecularModel), how bright the highlight is and its exponent.
constexpr const char* SPECULAR_MODEL_UNIFORM = "uSpecularModel";
constexpr const char* SPECULAR_STRENGTH_UNIFORM = "uSpecularStrength";
constexpr const char* SHININESS_UNIFORM = "uShininess";

/// common/lighting.glsl: the name of the uniform block with the lights, and the uniform
/// buffer binding point it is connected to. Every uniform block of a new program starts
/// at binding point 0. The lights use 1 on purpose: a program whose block was never
/// connected then reads from binding point 0, where no buffer is attached. The OpenGL
/// 4.1 specification leaves the values it gets undefined, so the mistake shows as wrong
/// lighting instead of the program working by accident.
constexpr const char* LIGHT_BLOCK_NAME = "LightBlock";
constexpr GLuint LIGHT_BLOCK_BINDING_POINT = 1;

/// common/shadows.glsl, so lit.frag, gouraud.frag and grass.frag: the names of the
/// uniforms of ONE shadow map. Every light that casts shadows (the moon and the
/// flashlight) has a set of its own in that file, and a constant of this type here
/// (game::setShadowUniforms takes it).
struct ShadowUniformNames {
    /// The sampler2DShadow of the map (it holds the number of a texture unit).
    const char* map;
    /// Whether the map is read (1) or nothing is in shadow (0).
    const char* enabled;
    /// World space to the clip space of the light (scene::LightSpace::matrix).
    const char* matrix;
    /// The two parts of the bias, as game::biasForShader gives them: differences of
    /// stored depths for the map of the moon, metres for the map of the flashlight.
    const char* constantBias;
    const char* slopeBias;
    /// The radius of the PCF kernel in texels. 0: one comparison.
    const char* pcfRadius;
    /// The share of the light a shadow takes away, 0 to 1.
    const char* strength;
    /// Where the light stands in world space (scene::LightSpace::position). Only the
    /// map of a light that has a position has this uniform. nullptr: there is none.
    const char* lightPosition;
};

/// The shadow map of the moon. A directional light has no position.
constexpr ShadowUniformNames MOON_SHADOW_UNIFORMS{
    .map = "uMoonShadowMap",
    .enabled = "uMoonShadowEnabled",
    .matrix = "uMoonShadowMatrix",
    .constantBias = "uMoonShadowConstantBias",
    .slopeBias = "uMoonShadowSlopeBias",
    .pcfRadius = "uMoonShadowPcfRadius",
    .strength = "uMoonShadowStrength",
    .lightPosition = nullptr,
};

/// The shadow map of the flashlight.
constexpr ShadowUniformNames FLASHLIGHT_SHADOW_UNIFORMS{
    .map = "uFlashlightShadowMap",
    .enabled = "uFlashlightShadowEnabled",
    .matrix = "uFlashlightShadowMatrix",
    .constantBias = "uFlashlightShadowConstantBias",
    .slopeBias = "uFlashlightShadowSlopeBias",
    .pcfRadius = "uFlashlightShadowPcfRadius",
    .strength = "uFlashlightShadowStrength",
    .lightPosition = "uFlashlightShadowLightPosition",
};

/// The texture units of the two shadow maps. The models use units 0 (colour picture)
/// and 1 (normal map) in the lit programs, and the composite pass uses 0 to 2, so 3 is
/// the first unit nothing else binds, and 4 the next. A map is bound once per frame and
/// stays on its unit while everything lit is drawn. One more shadow map takes the next
/// unit.
constexpr GLuint MOON_SHADOW_TEXTURE_UNIT = 3;
constexpr GLuint FLASHLIGHT_SHADOW_TEXTURE_UNIT = 4;

/// color.frag: the one colour of everything drawn, a linear colour.
constexpr const char* COLOR_UNIFORM = "uColor";

/// post/composite.frag: the sampler of the scene picture (it holds the number of
/// a texture unit), the exposure and the tone mapping curve (a value of
/// game::ToneMapping).
constexpr const char* COMPOSITE_SCENE_UNIFORM = "uScene";
constexpr const char* COMPOSITE_EXPOSURE_UNIFORM = "uExposure";
constexpr const char* COMPOSITE_TONE_MAPPING_UNIFORM = "uToneMapping";

/// post/composite.frag: the sampler of the blurred bloom picture (the number of a second
/// texture unit), whether it is added to the scene (1) or not read at all (0), and the
/// number it is multiplied by.
constexpr const char* COMPOSITE_BLOOM_UNIFORM = "uBloom";
constexpr const char* COMPOSITE_BLOOM_ENABLED_UNIFORM = "uBloomEnabled";
constexpr const char* COMPOSITE_BLOOM_INTENSITY_UNIFORM = "uBloomIntensity";

/// post/composite.frag, the fog: whether it is mixed in (1) or the depth is not read at
/// all (0), the sampler of the depth texture of the scene (the number of a third
/// texture unit), the density per metre, the world height up to which the fog has that
/// density, how fast it thins out above (per metre) and its colour, a linear colour.
constexpr const char* COMPOSITE_FOG_ENABLED_UNIFORM = "uFogEnabled";
constexpr const char* COMPOSITE_DEPTH_UNIFORM = "uDepth";
constexpr const char* COMPOSITE_FOG_DENSITY_UNIFORM = "uFogDensity";
constexpr const char* COMPOSITE_FOG_BASE_HEIGHT_UNIFORM = "uFogBaseHeight";
constexpr const char* COMPOSITE_FOG_HEIGHT_FALLOFF_UNIFORM = "uFogHeightFalloff";
constexpr const char* COMPOSITE_FOG_COLOR_UNIFORM = "uFogColor";

/// post/composite.frag, for the fog too: the inverse of projection * view of the scene
/// and the position of the eye in world space (game::SceneView).
constexpr const char* COMPOSITE_INVERSE_VIEW_PROJECTION_UNIFORM = "uInverseViewProjection";
constexpr const char* COMPOSITE_EYE_UNIFORM = "uEye";

/// post/composite.frag, the vignette: whether the corners are darkened (1) or not (0),
/// how much light they lose and the distance from the middle of the screen at which
/// the darkening starts.
constexpr const char* COMPOSITE_VIGNETTE_ENABLED_UNIFORM = "uVignetteEnabled";
constexpr const char* COMPOSITE_VIGNETTE_STRENGTH_UNIFORM = "uVignetteStrength";
constexpr const char* COMPOSITE_VIGNETTE_RADIUS_UNIFORM = "uVignetteRadius";

/// post/bright.frag: the sampler of the scene picture and the brightness above which
/// light takes part in the bloom.
constexpr const char* BRIGHT_SCENE_UNIFORM = "uScene";
constexpr const char* BRIGHT_THRESHOLD_UNIFORM = "uThreshold";

/// post/blur.frag: the sampler of the picture to blur, the direction of the pass
/// (1 horizontal, 0 vertical) and the array of the kernel weights
/// (game::bloomBlurWeights).
constexpr const char* BLUR_SOURCE_UNIFORM = "uSource";
constexpr const char* BLUR_HORIZONTAL_UNIFORM = "uHorizontal";
constexpr const char* BLUR_WEIGHTS_UNIFORM = "uWeights";

/// post/preview.frag: the sampler of the attachment to show, what it is (a value of
/// game::AttachmentPreview), the clipping planes of the perspective view the depth was
/// drawn with (the camera, or the flashlight for its shadow map) and the distance in
/// metres the depth preview shows as white.
constexpr const char* PREVIEW_SOURCE_UNIFORM = "uSource";
constexpr const char* PREVIEW_MODE_UNIFORM = "uMode";
constexpr const char* PREVIEW_NEAR_UNIFORM = "uNear";
constexpr const char* PREVIEW_FAR_UNIFORM = "uFar";
constexpr const char* PREVIEW_DEPTH_RANGE_UNIFORM = "uDepthRange";

/// skybox.frag: the sampler of the cube map (it holds the number of a texture unit) and
/// the number the colour of the sky is multiplied by.
constexpr const char* SKYBOX_UNIFORM = "uSkybox";
constexpr const char* SKYBOX_BRIGHTNESS_UNIFORM = "uBrightness";

/// grass.geom: the clock of the wind in seconds, the height of the tallest blades in
/// metres and how far the wind pushes the tips (0: still air).
constexpr const char* GRASS_TIME_UNIFORM = "uTime";
constexpr const char* GRASS_BLADE_HEIGHT_UNIFORM = "uBladeHeight";
constexpr const char* GRASS_WIND_STRENGTH_UNIFORM = "uWindStrength";

/// grass.frag: whether the grass is lit by the lights of the scene (1) or shown at full
/// brightness (0).
constexpr const char* GRASS_LIT_UNIFORM = "uLit";

} // namespace game
