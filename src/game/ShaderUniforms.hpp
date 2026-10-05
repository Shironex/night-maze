// Names of the uniform variables of the shaders in assets/shaders, in one place.
// See docs/modules/gfx/uniforms.md
#pragma once

#include <glad/gl.h>

namespace game {

// A uniform is found by its name (gfx::Shader::setMat4 and the other setters), so each
// string below must be spelled exactly like the "uniform" line of the shader file. A
// name with a typo is not an error: OpenGL silently ignores it. Keeping the names here,
// once, means that the classes that draw cannot disagree about them.

/// The three matrices. Every vertex shader (basic, textured, color, lit, gouraud)
/// declares them under the same names.
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";

/// textured.frag, lit.frag and gouraud.frag: the sampler (it holds the number of
/// a texture unit) and the colour the texture is multiplied by.
constexpr const char* TEXTURE_UNIFORM = "uTexture";
constexpr const char* TINT_UNIFORM = "uTint";

/// textured.frag only: what to show (a value of game::ViewMode).
constexpr const char* VIEW_MODE_UNIFORM = "uViewMode";

/// lit.vert and gouraud.vert: the matrix that takes normals to world space
/// (scene::normalMatrix).
constexpr const char* NORMAL_MATRIX_UNIFORM = "uNormalMatrix";

/// common/lighting.glsl, so lit.frag and gouraud.vert: the highlight formula (a value of
/// game::SpecularModel), how bright the highlight is and its exponent.
constexpr const char* SPECULAR_MODEL_UNIFORM = "uSpecularModel";
constexpr const char* SPECULAR_STRENGTH_UNIFORM = "uSpecularStrength";
constexpr const char* SHININESS_UNIFORM = "uShininess";

/// common/lighting.glsl: the name of the uniform block with the lights, and the uniform
/// buffer binding point it is connected to. Every uniform block of a new program starts
/// at binding point 0. The lights use 1 on purpose: a program whose block was never
/// connected then reads no lights at all, which is noticed at once, instead of working
/// by accident.
constexpr const char* LIGHT_BLOCK_NAME = "LightBlock";
constexpr GLuint LIGHT_BLOCK_BINDING_POINT = 1;

/// color.frag: the one colour of everything drawn.
constexpr const char* COLOR_UNIFORM = "uColor";

} // namespace game
