// VideoDecoder: reads a video file frame by frame with the decoder of the operating
// system. The one interface the rest of the program sees of the platform code.
#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace video {

// The game brings no video decoder of its own. Windows and macOS both have one built
// in: Media Foundation and AVFoundation. Each is used through its own interface, in its
// own file (VideoDecoderWindows.cpp and VideoDecoderApple.mm), and both files implement
// the class below. Nothing outside of those two files includes a header of the
// operating system or knows which of the two is running.

/// One pixel of a decoded frame: blue, green, red and one unused byte, in this order.
/// It is the order both operating systems deliver, and OpenGL takes it as GL_BGRA.
constexpr std::size_t VIDEO_BYTES_PER_PIXEL = 4;

/// What a decoder knows about its file once it is open.
struct VideoFormat {
    /// The size of a frame in pixels.
    int width = 0;
    int height = 0;

    /// How long one frame is shown, in seconds: 1 / 30 for 30 frames per second.
    double frameSeconds = 1.0 / 30.0;
};

/// How a call of VideoDecoder::readFrame ended.
enum class VideoRead {
    Frame,     ///< a frame was decoded: pixels and stampSeconds are filled
    EndOfFile, ///< the last frame was read before: nothing was filled, rewind goes on
    Failed,    ///< the decoder gave up: nothing was filled, and the file cannot be played
};

/// An open video file. It hands out the frames in the order they are shown.
///
/// A decoder belongs to ONE thread: the thread that called openVideoDecoder makes every
/// later call and destroys the object. Media Foundation is started for a thread, and
/// keeping to this rule means no part of a decoder ever needs a lock.
class VideoDecoder {
public:
    VideoDecoder() = default;
    virtual ~VideoDecoder() = default;

    VideoDecoder(const VideoDecoder&) = delete;
    VideoDecoder& operator=(const VideoDecoder&) = delete;

    /// The size of the frames and the time one of them is shown.
    virtual VideoFormat format() const = 0;

    /// Decodes the next frame. On VideoRead::Frame, pixels holds width * height *
    /// VIDEO_BYTES_PER_PIXEL bytes, row by row without gaps, TOP row first, and
    /// stampSeconds the time the file gives the frame, counted from the start of the
    /// file. pixels is resized as needed: handing in a vector of the right size again
    /// and again decodes without allocating memory.
    ///
    /// The call blocks until the frame is decoded (a few milliseconds).
    virtual VideoRead readFrame(std::vector<unsigned char>& pixels, double& stampSeconds) = 0;

    /// Goes back to the first frame of the file: the next readFrame decodes it. False
    /// when that failed. The call itself is short, but the readFrame after it is the
    /// slow one (measured on Windows: 16 to 37 milliseconds, against less than one for
    /// any other frame, because the decoder starts again at a key frame). That is why
    /// a player keeps both away from the thread that draws.
    virtual bool rewind() = 0;
};

/// Opens a video file with the decoder of the operating system. The file is an MP4 with
/// H.264 video, the one format both systems decode without anything installed.
///
/// Returns the decoder, or nullptr with one sentence in error: the decoder of the
/// system is missing (a Windows N edition without the Media Feature Pack), the file is
/// not a video it can decode, or a step of setting it up failed. It does not throw and
/// it does not log.
std::unique_ptr<VideoDecoder> openVideoDecoder(const std::filesystem::path& file,
                                               std::string& error);

} // namespace video
