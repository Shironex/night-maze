// CoverFit: which part of a picture is shown when it has to cover a target of another
// shape.
#pragma once

namespace gfx {

// Plain math without OpenGL, so tests can use it.

/// A rectangle of a picture in texture coordinates: 0 is the left edge and the first
/// row, 1 the right edge and the last row. The default is the whole picture.
struct UvRect {
    float left = 0.0F;
    float top = 0.0F;
    float right = 1.0F;
    float bottom = 1.0F;
};

/// The part of a picture that fills a target completely without being stretched: the
/// "cover" rule of a desktop wallpaper. The picture is scaled until it covers the whole
/// target, and what sticks out on two opposite sides is cut off, the same amount on
/// each, so the middle of the picture stays in the middle of the target.
///
/// A target that is wider than the picture (21 : 9 against 16 : 9) keeps the full width
/// and loses rows at the top and at the bottom. A target that is narrower (4 : 3) keeps
/// the full height and loses columns on the left and on the right. The same shape gives
/// the whole picture.
///
/// Only the shapes matter, not the sizes: 1280 x 720 into 1920 x 1080 is the whole
/// picture. A size below 1 in any of the four numbers gives the whole picture too.
UvRect coverFit(int pictureWidth, int pictureHeight, int targetWidth, int targetHeight);

} // namespace gfx
