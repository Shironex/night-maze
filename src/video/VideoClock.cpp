// VideoClock: the arithmetic of playing a video in a loop: when a frame is due, and
// what time a frame has on a time line that never jumps back.
#include "video/VideoClock.hpp"

#include <algorithm>

namespace video {

double timelineSeconds(double passStartSeconds, double frameStampSeconds,
                       double firstStampSeconds) {
    return passStartSeconds + (frameStampSeconds - firstStampSeconds);
}

double passSeconds(double firstStampSeconds, double lastStampSeconds, double frameSeconds) {
    return (lastStampSeconds - firstStampSeconds) + frameSeconds;
}

std::size_t dueFrameCount(std::span<const double> waitingSeconds, double clockSeconds) {
    std::size_t count = 0;
    // The list is in the order of showing, so the first frame that is not due yet ends
    // the search.
    while (count < waitingSeconds.size() && waitingSeconds[count] <= clockSeconds) {
        ++count;
    }
    return count;
}

double clockWithEmptyQueue(double clockSeconds, double deltaSeconds, double shownSeconds,
                           double frameSeconds) {
    return std::min(clockSeconds + deltaSeconds, shownSeconds + frameSeconds);
}

} // namespace video
