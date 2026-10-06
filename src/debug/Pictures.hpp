// Pictures of the debug window: a framebuffer shown as an image with a caption.
// See docs/modules/debug-ui.md
#pragma once

namespace gfx {
class Framebuffer;
} // namespace gfx

namespace debug {

/// Draws the colour texture of a framebuffer as a picture, width pixels wide, in the
/// shape of the framebuffer, with a caption above it and a thin outline around it.
/// tooltip says what the picture shows.
///
/// With drawn false the picture is not up to date (the pass that fills it did not run
/// in this frame), and a note stands in its place. The same note, worded differently,
/// stands there while the framebuffer does not exist yet: the first frame after the
/// picture was asked for.
///
/// Used for the previews of the Post process category, for the two shadow maps and for
/// the minimap. Call it inside a card of a Page, after Page::beginBlock returned true.
void drawFramebufferPicture(const char* caption, const char* tooltip,
                            const gfx::Framebuffer& picture, float width, bool drawn);

} // namespace debug
