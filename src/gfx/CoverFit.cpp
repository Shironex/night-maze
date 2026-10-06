// CoverFit: which part of a picture is shown when it has to cover a target of another
// shape.
#include "gfx/CoverFit.hpp"

namespace gfx {

namespace {

// Half of what is cut off goes to each of the two sides.
constexpr float HALF = 0.5F;

} // namespace

UvRect coverFit(int pictureWidth, int pictureHeight, int targetWidth, int targetHeight) {
    UvRect shown;
    if (pictureWidth < 1 || pictureHeight < 1 || targetWidth < 1 || targetHeight < 1) {
        return shown;
    }

    // Width divided by height: 1.78 for 16 : 9. The casts make it a division of floats.
    const float pictureAspect =
        static_cast<float>(pictureWidth) / static_cast<float>(pictureHeight);
    const float targetAspect = static_cast<float>(targetWidth) / static_cast<float>(targetHeight);

    if (targetAspect > pictureAspect) {
        // The target is wider: the full width is shown, and of the height only the
        // share that gives the shown part the shape of the target.
        const float shownShare = pictureAspect / targetAspect;
        shown.top = (1.0F - shownShare) * HALF;
        shown.bottom = shown.top + shownShare;
    } else {
        // The target is narrower (or the same): the full height, a share of the width.
        const float shownShare = targetAspect / pictureAspect;
        shown.left = (1.0F - shownShare) * HALF;
        shown.right = shown.left + shownShare;
    }
    return shown;
}

} // namespace gfx
