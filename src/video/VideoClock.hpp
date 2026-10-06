// VideoClock: the arithmetic of playing a video in a loop: when a frame is due, and
// what time a frame has on a time line that never jumps back.
#pragma once

#include <cstddef>
#include <span>

namespace video {

// Plain math without OpenGL, without threads and without a decoder, so tests can use it.
// video::VideoPlayer does what these functions say.
//
// Two clocks meet in a player. The file stamps every frame with the moment it is meant
// to be shown, counted from the start of the file (30 frames per second: 0, 1/30, 2/30
// and so on). The game has the time of its own frames, which come at any rate (60, 144
// or 20 per second). The video must follow its own stamps: a faster game shows every
// video frame for several of its frames, a slower game leaves some video frames out.

/// The time of a frame on the time line of the player, in seconds.
///
/// The stamps of the file start again at the beginning of every pass through the loop.
/// The time line of the player does not: it goes on, pass after pass, so "is this frame
/// due?" stays one comparison of two numbers also where the loop starts again.
///
/// passStartSeconds is where the pass of the frame begins on the time line (0 for the
/// first pass), frameStampSeconds the stamp of the frame in the file and
/// firstStampSeconds the stamp of the first frame of the file, which need not be 0.
double timelineSeconds(double passStartSeconds, double frameStampSeconds, double firstStampSeconds);

/// How long one pass through the file takes, in seconds: from the stamp of its first
/// frame to the stamp of its last one, plus the time the last frame itself is shown.
/// The next pass begins that much later on the time line, so the first frame follows
/// the last one after exactly one frame time, like every other pair of frames.
double passSeconds(double firstStampSeconds, double lastStampSeconds, double frameSeconds);

/// How many of the waiting frames are due at clockSeconds: the frames from the front of
/// the list whose time has come (time <= clock). waitingSeconds are the times of the
/// decoded frames that were not shown yet, in the order they will be shown.
///
/// The player takes that many frames out of its queue and shows the last of them. More
/// than one means the game is slower than the video: the frames before the last are
/// left out. Zero means the frame on screen stays.
std::size_t dueFrameCount(std::span<const double> waitingSeconds, double clockSeconds);

/// The clock after it was moved on by deltaSeconds, for a player whose queue of decoded
/// frames is empty. shownSeconds is the time of the frame on screen and frameSeconds
/// how long one frame is shown.
///
/// With an empty queue the decoder is behind (or the game stood still for a while), and
/// the clock must not run away from the picture: otherwise the frames that arrive later
/// would all be overdue at once and the video would jump forward. So the clock waits at
/// the moment the next frame is due, and the video goes on from where it stopped.
double clockWithEmptyQueue(double clockSeconds, double deltaSeconds, double shownSeconds,
                           double frameSeconds);

} // namespace video
