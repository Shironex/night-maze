// VideoPlayer: plays one video file in a loop into a texture.
#include "video/VideoPlayer.hpp"

#include "video/VideoClock.hpp"

#include <array>
#include <span>
#include <system_error>
#include <utility>

namespace video {

VideoPlayer::VideoPlayer(const std::filesystem::path& file) {
    // The decoding thread gets its own copy of the path: it runs on after this
    // constructor has returned. An exception that leaves the function of a thread ends
    // the whole program, so whatever is thrown in there (no memory for a frame, for
    // example) is caught and becomes a failed video. The three dots catch everything.
    //
    // clang-tidy still sees one way out for an exception: locking the mutex inside
    // giveUpAfterException. A mutex that cannot be locked means the program is broken
    // beyond repair, and ending it is then the right thing, so the check is told so.
    // NOLINTNEXTLINE(bugprone-exception-escape)
    m_thread = std::thread([this, file]() {
        try {
            decodeLoop(file);
        } catch (...) {
            giveUpAfterException();
        }
    });

    // Wait for the first frame, or for the news that there will be none. wait() gives
    // the mutex up while it sleeps and holds it again when it returns, and it checks
    // the condition itself every time it is woken.
    Frame first;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_changed.wait(lock, [this]() { return m_failed || !m_waiting.empty(); });
        if (m_failed) {
            return;
        }
        first = std::move(m_waiting.front());
        m_waiting.pop_front();
    }
    // There is room in the queue again.
    m_changed.notify_all();

    // The texture is created here and not on the decoding thread: an OpenGL context
    // belongs to one thread, the one that draws.
    m_texture = std::make_unique<gfx::FrameTexture>(m_format.width, m_format.height);
    m_texture->upload(first.pixels.data(), gfx::PixelOrder::Bgra);
    m_clockSeconds = first.seconds;
    m_shownSeconds = first.seconds;

    const std::lock_guard<std::mutex> lock(m_mutex);
    m_spare.push_back(std::move(first.pixels));
}

VideoPlayer::~VideoPlayer() {
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_changed.notify_all();
    // The thread notices m_stop before its next frame, a few milliseconds at most.
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool VideoPlayer::isPlaying() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_texture != nullptr && !m_failed;
}

std::string VideoPlayer::error() const {
    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_failed && m_error.empty()) {
        return "the decoding thread stopped with an exception";
    }
    return m_error;
}

void VideoPlayer::update(double deltaSeconds) {
    if (m_texture == nullptr) {
        return;
    }

    // The frame that goes into the texture in this call, if one is due.
    Frame newest;
    bool hasNewest = false;
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        if (m_failed) {
            return;
        }
        const double clockBefore = m_clockSeconds;
        m_clockSeconds = clockBefore + deltaSeconds;

        // The times of the waiting frames, for the question how many of them are due.
        std::array<double, QUEUE_CAPACITY> times{};
        std::size_t count = 0;
        for (const Frame& frame : m_waiting) {
            times[count] = frame.seconds;
            ++count;
        }
        const std::size_t due =
            dueFrameCount(std::span<const double>(times.data(), count), m_clockSeconds);

        // All of them leave the queue. Only the last one is shown: a frame before it
        // is a frame the game was too slow for, and its buffer goes straight back.
        for (std::size_t i = 0; i < due; ++i) {
            if (hasNewest) {
                m_spare.push_back(std::move(newest.pixels));
            }
            newest = std::move(m_waiting.front());
            m_waiting.pop_front();
            hasNewest = true;
        }
        if (hasNewest) {
            m_shownSeconds = newest.seconds;
        }
        // Nothing left to show after this: the clock must not run ahead of the decoder.
        if (m_waiting.empty()) {
            m_clockSeconds = clockWithEmptyQueue(clockBefore, deltaSeconds, m_shownSeconds,
                                                 m_format.frameSeconds);
        }
    }
    if (!hasNewest) {
        return;
    }
    // The decoding thread has room again. It is woken before the upload, so it decodes
    // while this thread copies the pixels to the graphics card (outside of the lock:
    // neither thread ever waits for the slow work of the other).
    m_changed.notify_all();
    m_texture->upload(newest.pixels.data(), gfx::PixelOrder::Bgra);

    const std::lock_guard<std::mutex> lock(m_mutex);
    m_spare.push_back(std::move(newest.pixels));
}

void VideoPlayer::bind(GLuint unit) const {
    if (m_texture != nullptr) {
        m_texture->bind(unit);
    }
}

void VideoPlayer::giveUp(const std::string& error) {
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_failed = true;
        m_error = error;
    }
    m_changed.notify_all();
}

void VideoPlayer::giveUpAfterException() {
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_failed = true;
    }
    m_changed.notify_all();
}

void VideoPlayer::decodeLoop(const std::filesystem::path& file) {
    // A missing file gets a sentence of its own: it is the most likely failure, and
    // the error of the operating system for it is only a number.
    std::error_code ignored;
    if (!std::filesystem::is_regular_file(file, ignored)) {
        giveUp("the file is missing");
        return;
    }

    // The decoder lives in this function: it is created, used and destroyed by this
    // thread alone (see VideoDecoder).
    std::string error;
    const std::unique_ptr<VideoDecoder> decoder = openVideoDecoder(file, error);
    if (decoder == nullptr) {
        giveUp(error);
        return;
    }
    // Written before the first frame is put into the queue. The constructor reads it
    // only after it has seen that frame.
    m_format = decoder->format();

    // Where the present pass through the file begins on the time line of the player,
    // and the stamps of its first frame and of the last frame read so far.
    double passStartSeconds = 0.0;
    double firstStampSeconds = 0.0;
    double lastStampSeconds = 0.0;
    bool passHasFrames = false;

    // The buffer the next frame is decoded into.
    std::vector<unsigned char> pixels;
    while (true) {
        {
            // Sleep until there is room in the queue, or until the player is destroyed.
            std::unique_lock<std::mutex> lock(m_mutex);
            m_changed.wait(lock, [this]() { return m_stop || m_waiting.size() < QUEUE_CAPACITY; });
            if (m_stop) {
                return;
            }
            // A buffer of a frame that was shown is used again.
            if (pixels.empty() && !m_spare.empty()) {
                pixels = std::move(m_spare.back());
                m_spare.pop_back();
            }
        }

        // The slow part, outside of the lock: the drawing thread is never held up.
        double stampSeconds = 0.0;
        const VideoRead read = decoder->readFrame(pixels, stampSeconds);
        if (read == VideoRead::Failed) {
            giveUp("the decoder stopped in the middle of the file");
            return;
        }
        if (read == VideoRead::EndOfFile) {
            // The loop: back to the first frame. A file without a single frame would
            // go round here for ever, so it counts as a failure.
            if (!passHasFrames) {
                giveUp("the file holds no frames");
                return;
            }
            if (!decoder->rewind()) {
                giveUp("the decoder cannot go back to the first frame");
                return;
            }
            passStartSeconds +=
                passSeconds(firstStampSeconds, lastStampSeconds, m_format.frameSeconds);
            passHasFrames = false;
            continue;
        }

        if (!passHasFrames) {
            firstStampSeconds = stampSeconds;
            passHasFrames = true;
        }
        lastStampSeconds = stampSeconds;

        Frame frame;
        frame.seconds = timelineSeconds(passStartSeconds, stampSeconds, firstStampSeconds);
        frame.pixels = std::move(pixels);
        // Moved from: emptied, so the next turn takes a spare buffer or lets the
        // decoder allocate one.
        pixels.clear();
        {
            const std::lock_guard<std::mutex> lock(m_mutex);
            m_waiting.push_back(std::move(frame));
        }
        // The constructor may be waiting for this frame.
        m_changed.notify_all();
    }
}

} // namespace video
