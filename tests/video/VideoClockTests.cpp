// Tests of video/VideoClock.hpp: when a frame of a looping video is due.
#include "video/VideoClock.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

namespace {

// One frame of a video with 30 frames per second, in seconds.
constexpr double FRAME_SECONDS = 1.0 / 30.0;

// The times of count frames on the time line of a player that plays a file of
// framesPerPass frames in a loop: what the decoding thread of video::VideoPlayer
// computes, pass after pass. firstStamp is the stamp of the first frame of the file.
std::vector<double> loopTimes(int count, int framesPerPass, double firstStamp) {
    std::vector<double> times;
    double passStart = 0.0;
    for (int i = 0; i < count; ++i) {
        const int inPass = i % framesPerPass;
        const double stamp = firstStamp + inPass * FRAME_SECONDS;
        times.push_back(video::timelineSeconds(passStart, stamp, firstStamp));
        if (inPass == framesPerPass - 1) {
            passStart += video::passSeconds(firstStamp, stamp, FRAME_SECONDS);
        }
    }
    return times;
}

// Plays the frames with the given times for a game that draws a frame every
// gameFrameSeconds, and returns for every game frame the index of the video frame on
// screen. The queue is never empty here: every frame is decoded in time.
std::vector<std::size_t> play(const std::vector<double>& times, double gameFrameSeconds,
                              int gameFrames) {
    std::vector<std::size_t> shown;
    std::size_t next = 1; // frame 0 is on screen from the start
    double clock = 0.0;
    for (int i = 0; i < gameFrames; ++i) {
        clock += gameFrameSeconds;
        const std::span<const double> waiting(times.data() + next, times.size() - next);
        next += video::dueFrameCount(waiting, clock);
        shown.push_back(next - 1);
    }
    return shown;
}

} // namespace

TEST_CASE("the time line goes on where the stamps of the file start again") {
    // A file of 4 frames, played three times: 12 frames, one frame time apart, with no
    // gap and no step back at the two places where the loop starts again.
    const std::vector<double> times = loopTimes(12, 4, 0.0);
    for (std::size_t i = 0; i < times.size(); ++i) {
        CHECK(times[i] == doctest::Approx(static_cast<double>(i) * FRAME_SECONDS));
    }
}

TEST_CASE("a file whose first stamp is not zero plays the same") {
    // Files with reordered frames often start a little after zero.
    const std::vector<double> late = loopTimes(12, 4, 0.0667);
    const std::vector<double> zero = loopTimes(12, 4, 0.0);
    for (std::size_t i = 0; i < late.size(); ++i) {
        CHECK(late[i] == doctest::Approx(zero[i]));
    }
}

TEST_CASE("one pass takes as long as its frames are shown") {
    // 720 frames at 30 per second: 24 seconds. The stamps run from 0 to 719 / 30.
    CHECK(video::passSeconds(0.0, 719.0 * FRAME_SECONDS, FRAME_SECONDS) == doctest::Approx(24.0));
    // One frame alone is shown for one frame time.
    CHECK(video::passSeconds(5.0, 5.0, FRAME_SECONDS) == doctest::Approx(FRAME_SECONDS));
}

TEST_CASE("the frames whose time has come are due, and no others") {
    const std::vector<double> waiting = {1.0, 1.1, 1.2, 1.3};
    CHECK(video::dueFrameCount(waiting, 0.9) == 0);
    // Exactly at its time a frame is due.
    CHECK(video::dueFrameCount(waiting, 1.0) == 1);
    CHECK(video::dueFrameCount(waiting, 1.25) == 3);
    CHECK(video::dueFrameCount(waiting, 99.0) == 4);
    CHECK(video::dueFrameCount({}, 99.0) == 0);
}

TEST_CASE("a game twice as fast as the video shows every video frame twice") {
    const std::vector<double> times = loopTimes(40, 10, 0.0);
    // 60 game frames per second against 30 video frames. The small number is added so
    // that rounding does not decide whether a frame is due exactly at its time.
    const std::vector<std::size_t> shown = play(times, FRAME_SECONDS / 2.0 + 1e-9, 60);
    for (std::size_t i = 0; i < shown.size(); ++i) {
        // The video frames on screen are 0, 1, 1, 2, 2, 3, 3 and so on: after the first
        // game frame every video frame stays for two, also across the loop starts.
        CHECK(shown[i] == (i + 1) / 2);
    }
}

TEST_CASE("a game slower than the video leaves frames out and keeps the speed") {
    const std::vector<double> times = loopTimes(90, 30, 0.0);
    // 20 game frames per second: one and a half video frames pass per game frame.
    const std::vector<std::size_t> shown = play(times, FRAME_SECONDS * 1.5 + 1e-9, 40);
    // After 40 game frames (2 seconds) the video is at frame 60: two seconds in.
    CHECK(shown.back() == 60);
    // It never goes back, and never jumps by more than two frames.
    for (std::size_t i = 1; i < shown.size(); ++i) {
        CHECK(shown[i] > shown[i - 1]);
        CHECK(shown[i] - shown[i - 1] <= 2);
    }
}

TEST_CASE("with an empty queue the clock waits for the next frame") {
    // The frame on screen has the time 2.0. The clock may run up to the moment the
    // next frame is due, and no further.
    const double due = 2.0 + FRAME_SECONDS;
    CHECK(video::clockWithEmptyQueue(2.0, 0.01, 2.0, FRAME_SECONDS) == doctest::Approx(2.01));
    CHECK(video::clockWithEmptyQueue(2.0, 0.25, 2.0, FRAME_SECONDS) == doctest::Approx(due));
    CHECK(video::clockWithEmptyQueue(due, 0.25, 2.0, FRAME_SECONDS) == doctest::Approx(due));

    // So a frame that arrives late is shown at once, and the one after it one frame
    // time later: the video goes on from where it stopped and does not jump ahead.
    const double clock = video::clockWithEmptyQueue(2.0, 5.0, 2.0, FRAME_SECONDS);
    const std::vector<double> arrived = {due, due + FRAME_SECONDS};
    CHECK(video::dueFrameCount(arrived, clock) == 1);
}
