// Names of the uniform variables of the shaders in assets/shaders, in one place.
// See docs/modules/gfx/uniforms.md
#pragma once

namespace game {

// A uniform is found by its name (gfx::Shader::setMat4 and the other setters), so each
// string below must be spelled exactly like the "uniform" line of the shader file. A
// name with a typo is not an error: OpenGL silently ignores it. Keeping the names here,
// once, means that the classes that draw cannot disagree about them.

/// The three matrices. basic.vert, textured.vert and color.vert all declare them under
/// the same names.
constexpr const char* MODEL_UNIFORM = "uModel";
constexpr const char* VIEW_UNIFORM = "uView";
constexpr const char* PROJECTION_UNIFORM = "uProjection";

/// textured.frag: the sampler (it holds the number of a texture unit), the colour the
/// texture is multiplied by, and what to show (a value of game::ViewMode).
constexpr const char* TEXTURE_UNIFORM = "uTexture";
constexpr const char* TINT_UNIFORM = "uTint";
constexpr const char* VIEW_MODE_UNIFORM = "uViewMode";

/// color.frag: the one colour of everything drawn.
constexpr const char* COLOR_UNIFORM = "uColor";

} // namespace game
