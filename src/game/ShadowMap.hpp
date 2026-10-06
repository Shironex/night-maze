// ShadowMap: the depth texture a light draws the scene into, and how the lit shaders read it.
// See docs/modules/renderer/shadows.md
#pragma once

#include "gfx/ComparisonSampler.hpp"
#include "gfx/Framebuffer.hpp"
#include "gfx/VertexArray.hpp"

#include <glad/gl.h>

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct LightSpace;
} // namespace scene

namespace game {

struct ShadowSettings;
struct ShadowUniformNames;

/// The OpenGL side of one shadow map: a square framebuffer without a colour texture
/// (only depth is drawn), the comparison sampler its depth texture is read through, and
/// a small preview picture for the debug UI. The math (where the light looks, the bias)
/// is plain data in game/Shadows.hpp and scene/LightSpace.hpp.
///
/// One object per light that casts shadows: the moon has one and the flashlight has
/// one.
///
/// A frame with it:
///   1. beginDepthPass: the map becomes the target, with the viewport set to its size
///      and its depth cleared.
///   2. The game draws everything that casts a shadow with the depth program
///      (shadow_depth.vert and shadow_depth.frag), from the view of the light.
///   3. bindForSampling: the depth texture and the comparison sampler go to a texture
///      unit, where the lit programs find them for the rest of the frame.
///   4. drawPreview (only for the debug UI).
/// After that another framebuffer has to be bound (the scene framebuffer): this class
/// leaves its own bound, and the viewport at the size of the map or of the preview.
///
/// It owns OpenGL objects, so it must be destroyed before the window.
class ShadowMap {
public:
    /// Creates the comparison sampler and the vertex array of the preview triangle. The
    /// framebuffer is created by the first beginDepthPass, when the size is known.
    ShadowMap() = default;

    /// Makes the shadow map the target of the draw calls that follow: a square of size
    /// by size texels (created again when size is not the one it has), the viewport set
    /// to it, the depth test switched on and the depth cleared to 1, the far plane.
    ///
    /// Returns false when there is nothing to draw into: the framebuffer could not be
    /// created (logged once per size). The shadows must then be skipped for the frame.
    bool beginDepthPass(int size);

    /// Binds the depth texture and the comparison sampler to texture unit number unit,
    /// for a sampler2DShadow uniform set to the same number. linearFilter chooses the
    /// filter of the sampler (gfx::ComparisonSampler::setLinearFilter). Texture unit
    /// 0 is the active one afterwards, as the code that binds the textures of the
    /// models expects.
    void bindForSampling(GLuint unit, bool linearFilter);

    /// Draws the preview picture: the depths of the map as greys, from black at the
    /// light to white at its far plane, with the preview program (post/composite.vert
    /// and post/preview.frag). lightSpace is the one the map was drawn with: it tells
    /// how a stored depth becomes a grey.
    ///
    /// Orthographic (the moon): the stored depths grow evenly with the distance, so
    /// they are shown as they are (AttachmentPreview::RawDepth).
    /// Perspective (the flashlight): stored as they are, almost every depth is close to
    /// 1 and the picture would be nearly white. They are turned back into metres with
    /// the near and the far plane of the light and shown as a share of its far plane
    /// (AttachmentPreview::Depth, the mode the depth of the scene is shown with).
    ///
    /// Call it after the depth pass. It switches the depth test off and leaves the
    /// preview framebuffer bound.
    void drawPreview(const gfx::Shader& previewShader, const scene::LightSpace& lightSpace);

    /// The framebuffer of the map, for the debug UI (its size and format). Not valid
    /// before the first successful beginDepthPass.
    const gfx::Framebuffer& target() const { return m_target; }

    /// The preview picture: a small GL_RGBA8 framebuffer whose colour texture can be
    /// shown as it is. Not valid before the first drawPreview.
    const gfx::Framebuffer& preview() const { return m_preview; }

private:
    // The depth-only framebuffer: the map.
    gfx::Framebuffer m_target;
    // The size the last beginDepthPass asked for. Kept apart from the size of m_target,
    // so that a creation that failed is not tried again in every frame.
    int m_requestedSize = 0;

    // How the lit programs read the map: with a comparison (see the class).
    gfx::ComparisonSampler m_sampler;

    // The preview picture and the vertex array of the triangle it is drawn with. The
    // triangle has no vertex data (post/composite.vert computes its corners), but
    // a Core profile refuses to draw without a vertex array object bound.
    gfx::Framebuffer m_preview;
    gfx::VertexArray m_triangle;
};

/// Sets the uniforms one shadow map has in common/shadows.glsl, in the program shader,
/// which must be in use. names are the names of those uniforms (MOON_SHADOW_UNIFORMS or
/// FLASHLIGHT_SHADOW_UNIFORMS in game/ShaderUniforms.hpp) and unit is the texture unit
/// of bindForSampling.
///
/// Call it in every frame for every program that includes common/shadows.glsl, also
/// when the shadows are switched off: after a shader reload every uniform is back at 0,
/// and a shadow sampler left at unit 0 would share that unit with the colour texture.
/// OpenGL refuses to draw with two samplers of different kinds on one unit.
///
/// drawn says whether the depth pass has filled the map in this frame. When it is false
/// the shaders do not read the map at all. The bias of the settings is in metres and is
/// handed over as game::biasForShader gives it for lightSpace (a difference of stored
/// depths for the box of the moon, metres for the pyramid of the flashlight), the PCF
/// radius as game::pcfRadiusInUse. A set of names with a lightPosition also gets the
/// position of lightSpace.
void setShadowUniforms(const gfx::Shader& shader, const ShadowUniformNames& names, GLuint unit,
                       bool drawn, const ShadowSettings& settings,
                       const scene::LightSpace& lightSpace);

} // namespace game
