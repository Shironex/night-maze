// VideoPlayer: plays one video file in a loop into a texture.
#pragma once

#include "gfx/FrameTexture.hpp"
#include "video/VideoDecoder.hpp"

#include <glad/gl.h>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace video {

/// Plays a video file in a loop. The picture of the moment is in a texture, which
/// whoever owns the player draws as it likes.
///
/// Two threads work together, and each has one job:
///
///   - The DECODING thread, started by the constructor, owns the decoder
///     (video::VideoDecoder). It decodes the frames one after the other and puts them
///     into a short queue: QUEUE_CAPACITY frames ahead of the picture. When the queue
///     is full it sleeps. At the end of the file it goes back to the first frame and
///     goes on: for the queue the loop never ends.
///   - The DRAWING thread (the thread that owns the OpenGL context and calls update)
///     keeps the clock. In every frame of the game it moves the clock on by the time
///     that passed, takes the frames whose time has come out of the queue and copies
///     the newest of them into the texture. It never waits for the decoder: when no
///     frame is due or none is ready, the picture in the texture stays.
///
/// Why a thread at all: decoding one frame takes about one millisecond, which the
/// drawing thread could afford. The first frame after going back to the start takes
/// 16 to 37 (the decoder starts again at a key frame), and on the drawing thread that
/// was one or two missed frames of the game at every pass of the loop. On the decoding
/// thread it costs nothing that can be seen: the queue holds enough frames to cover it.
///
/// The two threads share only the queue and a few flags, all guarded by one mutex. The
/// decoder is touched by the decoding thread alone and OpenGL by the drawing thread
/// alone.
///
/// The video follows its own clock, not the frame rate of the game (video/VideoClock.hpp).
/// While update is not called (the menu is closed) the clock stands still and the
/// decoding thread sleeps on its full queue: the video costs nothing and goes on later
/// where it stopped.
class VideoPlayer {
public:
    /// How many decoded frames wait at most. At 30 frames per second four frames are
    /// 133 milliseconds of video, several times the slowest thing the decoder does. In
    /// memory they are four pictures: 15 MB at 1280 x 720.
    static constexpr std::size_t QUEUE_CAPACITY = 4;

    /// Opens the file and waits until its first frame is decoded and in the texture, or
    /// until that failed (a tenth of a second, once). It needs a current OpenGL
    /// context. It does not throw and does not log: ask isPlaying() and error().
    explicit VideoPlayer(const std::filesystem::path& file);

    /// Stops the decoding thread and waits for it to end.
    ~VideoPlayer();

    VideoPlayer(const VideoPlayer&) = delete;
    VideoPlayer& operator=(const VideoPlayer&) = delete;

    /// True while the video can be shown. False when the file could not be opened, and
    /// from the moment the decoder gave up in the middle of playing: error() then says
    /// why, and the texture keeps its last picture.
    bool isPlaying() const;

    /// One sentence about what went wrong. Empty while the video plays.
    std::string error() const;

    /// The size of a frame in pixels. 0 x 0 when the file could not be opened.
    int width() const { return m_format.width; }
    int height() const { return m_format.height; }

    /// Moves the clock of the video on by deltaSeconds (the time since the last call:
    /// the time of one frame of the game) and puts the frame that is due into the
    /// texture. Call it once per drawn frame, on the thread with the OpenGL context.
    void update(double deltaSeconds);

    /// Binds the texture with the picture of the moment to texture unit number unit.
    /// Its first row is the TOP row of the picture. Does nothing without a texture.
    void bind(GLuint unit) const;

private:
    // One decoded frame: its pixels (VideoDecoder::readFrame) and its time on the time
    // line of the player (video::timelineSeconds).
    struct Frame {
        std::vector<unsigned char> pixels;
        double seconds = 0.0;
    };

    // The body of the decoding thread: opens the file, then decodes until m_stop.
    void decodeLoop(const std::filesystem::path& file);

    // Called by the decoding thread when it cannot go on: stores the sentence and
    // wakes the constructor, in case it is still waiting.
    void giveUp(const std::string& error);

    // The same after an exception, when there may be no memory left for a sentence:
    // it only sets the flag, and error() supplies the words.
    void giveUpAfterException();

    // ---- shared by both threads, guarded by m_mutex ---------------------------------
    mutable std::mutex m_mutex;
    // Signalled whenever something changed that the other thread may wait for: a frame
    // was added (the constructor waits for the first one), a frame was taken (the
    // decoding thread waits for room), the decoder gave up or the player is destroyed.
    std::condition_variable m_changed;
    // The decoded frames that were not shown yet, in the order they will be shown.
    std::deque<Frame> m_waiting;
    // Pixel buffers of frames that were shown, for the decoding thread to fill again:
    // no memory is allocated while the video plays.
    std::vector<std::vector<unsigned char>> m_spare;
    // Set once by the decoding thread when it cannot go on: the file could not be
    // opened, or the decoder gave up later. m_error says why.
    bool m_failed = false;
    std::string m_error;
    // Set by the destructor: the decoding thread ends.
    bool m_stop = false;

    // Written by the decoding thread before it puts the first frame into the queue and
    // never again, read by the drawing thread only after the constructor has taken
    // that frame.
    VideoFormat m_format;

    // ---- used by the drawing thread only ---------------------------------------------
    std::unique_ptr<gfx::FrameTexture> m_texture;
    // The clock of the video, and the time of the frame that is in the texture, both
    // in seconds on the time line of the player.
    double m_clockSeconds = 0.0;
    double m_shownSeconds = 0.0;

    // Declared last: the thread starts in the constructor and uses the members above.
    std::thread m_thread;
};

} // namespace video
