// ShadowMap: the depth texture a light draws the scene into, and how the lit shaders read it.
#pragma once

#include "game/Shadows.hpp"
#include "gfx/ComparisonSampler.hpp"
#include "gfx/Framebuffer.hpp"
#include "gfx/VertexArray.hpp"

#include <glad/gl.h>

#include <optional>

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct LightSpace;
} // namespace scene

namespace game {

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
/// A light that stands still relative to the world (the moon) can split step 1 and 2 in
/// two, so that the casters that never move are not drawn in every frame. The map then
/// has a second depth texture of the same size, the still layer:
///   1a. Only when staticLayerHolds says the layer is out of date: beginStaticPass, and
///       the game draws the casters that never move into the layer.
///   1b. beginDepthPassOverStatic: the map becomes the target, filled with a copy of the
///       layer in place of the clear.
///   2.  The game draws only the casters that can move. The depth test keeps the nearer
///       of the two at every texel, exactly as if all of them had been drawn in one pass:
///       the nearest depth of a set does not depend on the order it is found in.
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

    /// True when the still layer holds the casters that never move as key describes
    /// them: it was drawn with exactly this key and nothing has thrown it away since.
    /// False before the first beginStaticPass.
    bool staticLayerHolds(const StaticShadowKey& key) const { return m_staticKey == key; }

    /// Makes the still layer the target of the draw calls that follow: a square of
    /// key.mapSize texels (created again when that is not the size it has), the viewport
    /// set to it, the depth test switched on and the depth cleared to 1. The caller then
    /// draws every caster that never moves, and nothing else. key is remembered as what
    /// the layer holds from now on (staticLayerHolds).
    ///
    /// Returns false when the layer could not be created. It then holds nothing, and
    /// the shadows must be skipped for the frame.
    bool beginStaticPass(const StaticShadowKey& key);

    /// beginDepthPass for a map with a still layer: the map becomes the target, at the
    /// size of the layer, with the depths of the layer copied into it in place of the
    /// clear (gfx::Framebuffer::copyDepthTo). The caller then draws the casters that can
    /// move.
    ///
    /// Returns false when the layer holds nothing (no beginStaticPass has worked yet) or
    /// the map could not be created. The shadows must then be skipped for the frame.
    bool beginDepthPassOverStatic();

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
    /// Gives m_target the size asked for: created again when size is not the one the
    /// last call asked for. Returns false when there is no framebuffer to draw into.
    bool prepareTarget(int size);

    // The depth-only framebuffer: the map.
    gfx::Framebuffer m_target;
    // The size the last depth pass asked for. Kept apart from the size of m_target,
    // so that a creation that failed is not tried again in every frame.
    int m_requestedSize = 0;

    // The still layer: a second depth-only framebuffer of the size of the map, with the
    // casters that never move. Not valid in a map that is only used with beginDepthPass
    // (the flashlight): no object is created for it then.
    gfx::Framebuffer m_static;
    // The size the last beginStaticPass asked for, kept like m_requestedSize.
    int m_requestedStaticSize = 0;
    // What m_static holds: the key of the pass that drew it. Empty while it holds
    // nothing: before the first pass, and after a pass that could not create it.
    std::optional<StaticShadowKey> m_staticKey;

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
