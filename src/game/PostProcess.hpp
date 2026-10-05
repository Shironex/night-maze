// PostProcess: the HDR framebuffer the scene is drawn into and the composite pass that
// brings it to the window with exposure, tone mapping and gamma correction.
// See docs/modules/renderer/post-process.md
#pragma once

#include "core/Window.hpp"
#include "gfx/Framebuffer.hpp"
#include "gfx/VertexArray.hpp"

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

/// How the brightness range of the scene (0 to anything) is brought into the range of
/// the screen (0 to 1). The debug UI shows the entries in this order, and the numbers
/// are the values of the uniform uToneMapping in post/composite.frag.
enum class ToneMapping {
    None = 0,     ///< no curve: everything above 1 is cut off (clamped)
    Reinhard = 1, ///< x / (1 + x): nothing is cut off, the picture gets flatter
    Aces = 2,     ///< a fitted film curve: more contrast, bright values bend softly to 1
};

/// What a preview picture shows. The numbers are the values of the uniform uMode in
/// post/preview.frag.
enum class AttachmentPreview {
    Color = 0, ///< the HDR colour attachment
    Depth = 1, ///< the depth attachment, as a distance
};

/// What can be changed about the last pass of a frame while the game runs. The debug UI
/// edits the fields.
struct PostProcessSettings {
    /// The colours of the scene are multiplied by this number before tone mapping, like
    /// the exposure of a camera. 1 changes nothing. The default is part of the look of
    /// the night: the lights are set for it.
    float exposure = 1.0F;

    /// The curve that follows the exposure.
    ToneMapping toneMapping = ToneMapping::Aces;

    /// Whether the two preview pictures of the attachments are drawn in this frame.
    /// They cost two small passes, so they are made only while the Framebuffers panel
    /// of the debug UI is open: the debug UI sets this field every frame.
    bool previews = false;

    /// The depth preview shows distances from 0 to this many metres as black to white.
    float depthPreviewRange = 15.0F;
};

/// Owns the framebuffer the whole 3D scene is drawn into, and draws the passes that
/// follow the scene.
///
/// A frame with it:
///   1. beginScene: the scene framebuffer becomes the target. It has a floating point
///      colour texture (GL_RGBA16F) and a depth texture, both as large as the
///      framebuffer of the window.
///   2. The game clears it and draws the scene as before. The colours are linear and
///      may be brighter than 1.
///   3. drawPreviews (only for the debug UI): two small pictures of the attachments.
///   4. composite: the window becomes the target again, and one triangle over the whole
///      screen reads the colour texture and writes exposure, tone mapping and the sRGB
///      encoding. After it the debug UI is drawn straight into the window.
///
/// Passes that are added later have their place between 2 and 4: they read
/// sceneTarget() and draw into framebuffers of their own.
///
/// It owns OpenGL objects, so it must be destroyed before the window.
class PostProcess {
public:
    /// Creates the vertex array of the fullscreen triangle. The framebuffers are created
    /// by the first beginScene, when the size of the window is known.
    PostProcess() = default;

    /// Makes the scene framebuffer the target of the draw calls that follow, with the
    /// viewport set to its size. size is the framebuffer size of the window in pixels
    /// (never the window size: on a Retina display the two differ). When it is not the
    /// size of the scene framebuffer, the framebuffer is created again in that size.
    ///
    /// Returns false when there is nothing to draw into: size is 0 in a direction
    /// (a minimised window) or the framebuffer could not be created (logged once per
    /// size). The frame must then skip the scene and composite.
    bool beginScene(core::Size size);

    /// Draws the two preview pictures of the attachments of the scene framebuffer into
    /// small framebuffers of their own, with the preview program (post/composite.vert
    /// and post/preview.frag). nearPlane and farPlane are the clipping planes the scene
    /// was drawn with: the depth preview needs them to turn depth into metres.
    /// Call it after the scene is drawn. It leaves one of the preview framebuffers
    /// bound, so composite (or another bind) has to follow.
    void drawPreviews(const gfx::Shader& shader, const PostProcessSettings& settings,
                      float nearPlane, float farPlane);

    /// The last pass: makes the window the target again (viewport of size windowSize,
    /// its framebuffer size) and draws the scene picture into it with the composite
    /// program (post/composite.vert and post/composite.frag): exposure, tone mapping,
    /// sRGB encoding. It switches the depth test off and leaves it off.
    void composite(const gfx::Shader& shader, const PostProcessSettings& settings,
                   core::Size windowSize) const;

    /// The framebuffer of the scene, for passes that read its colour or its depth and
    /// for the debug UI. Not valid before the first successful beginScene.
    const gfx::Framebuffer& sceneTarget() const { return m_scene; }

    /// The preview picture of one attachment: a small GL_RGBA8 framebuffer whose colour
    /// texture can be shown as it is. Not valid before the first drawPreviews.
    const gfx::Framebuffer& preview(AttachmentPreview which) const {
        return which == AttachmentPreview::Depth ? m_depthPreview : m_colorPreview;
    }

private:
    /// Draws the triangle that covers the whole target, with the program in use.
    void drawFullscreenTriangle() const;

    // The HDR target of the scene.
    gfx::Framebuffer m_scene;
    // The size the last beginScene asked for. Kept apart from the size of m_scene, so
    // that a creation that failed is not tried again in every frame.
    core::Size m_requestedSize;

    // The preview pictures of the two attachments.
    gfx::Framebuffer m_colorPreview;
    gfx::Framebuffer m_depthPreview;

    // A vertex array without attributes. The fullscreen triangle has no vertex data
    // (post/composite.vert computes its corners), but a Core profile refuses to draw
    // without a vertex array object bound.
    gfx::VertexArray m_triangle;
};

} // namespace game
