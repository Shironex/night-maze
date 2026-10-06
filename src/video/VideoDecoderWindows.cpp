// VideoDecoder on Windows: frames of a video file through Media Foundation.
#include "video/VideoDecoder.hpp"

// The whole file is for Windows only. It is compiled on every system, so that the
// checks that run over all source files (make tidy) find it in the list of compiled
// files also on macOS, where nothing is left of it. The macOS decoder is in
// VideoDecoderApple.mm.
#ifdef _WIN32

// Keep windows.h small, and stop it from defining the min and max macros, which break
// std::min and std::max. The header is included only here, never in a .hpp file.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace video {

namespace {

// Media Foundation is the part of Windows that reads and decodes media files. Its
// objects are COM objects: a function creates one and fills in a pointer, every call
// answers with an HRESULT (SUCCEEDED or FAILED), and an object lives until its last
// user has called Release(). ComPtr is the smart pointer of Windows for them: it calls
// Release() in its destructor, like std::unique_ptr calls delete.
using Microsoft::WRL::ComPtr;

// Which stream of the file is read: its first video stream. And the name for "every
// stream". The constants of Media Foundation are signed enum values, and the functions
// take an unsigned number.
constexpr auto VIDEO_STREAM = static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
constexpr auto ALL_STREAMS = static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS);

// Media Foundation counts time in units of 100 nanoseconds: ten million per second.
constexpr double STAMP_UNITS_PER_SECOND = 10000000.0;

// The two libraries of Media Foundation this file calls into.
constexpr std::array<const wchar_t*, 2> MEDIA_FOUNDATION_LIBRARIES = {L"mfplat.dll",
                                                                      L"mfreadwrite.dll"};

// One sentence for a failed step, with the number Windows gave: the number can be
// looked up, "failed" alone cannot.
std::string failedStep(const char* step, HRESULT result) {
    std::array<char, 16> number{};
    std::snprintf(number.data(), number.size(), "0x%08lX", static_cast<unsigned long>(result));
    return std::string(step) + " (HRESULT " + number.data() + ")";
}

// True when the libraries of Media Foundation are on this computer. The N editions of
// Windows come without them until the Media Feature Pack is installed.
//
// The program does not load these two libraries when it starts, like it loads every
// other library of Windows: then it could not even start without them. They are
// "delay loaded" (the linker switch /DELAYLOAD, set in CMakeLists.txt): Windows loads
// them at the first call into them. A missing library would end the program at that
// call, so this function asks first.
bool mediaFoundationInstalled() {
    for (const wchar_t* name : MEDIA_FOUNDATION_LIBRARIES) {
        // Only the directory of Windows itself is searched, not the directory of the
        // game or the working directory.
        const HMODULE library = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (library == nullptr) {
            return false;
        }
        // The question is answered. The library is loaded again at the first call.
        FreeLibrary(library);
    }
    return true;
}

// Copies height rows of rowBytes bytes each into pixels, top row first. topRow points
// at the top row in the memory of the decoder and pitch is the distance from one row
// to the row below it, in bytes. The pitch can be larger than a row (padding at the
// end of every row) and it can be negative: Windows stores some pictures bottom row
// first, and the row below is then EARLIER in memory.
void copyRows(const BYTE* topRow, LONG pitch, std::size_t rowBytes, int height,
              std::vector<unsigned char>& pixels) {
    for (int row = 0; row < height; ++row) {
        const BYTE* source = topRow + static_cast<std::ptrdiff_t>(pitch) * row;
        std::memcpy(pixels.data() + rowBytes * static_cast<std::size_t>(row), source, rowBytes);
    }
}

class MediaFoundationDecoder final : public VideoDecoder {
public:
    MediaFoundationDecoder() = default;

    ~MediaFoundationDecoder() override {
        // The reader has to go before Media Foundation is shut down, and Media
        // Foundation before COM: the reverse of the order they were started in.
        m_reader.Reset();
        if (m_mediaFoundationStarted) {
            MFShutdown();
        }
        if (m_comStarted) {
            CoUninitialize();
        }
    }

    // Opens the file and asks for frames as plain pixels. False, with a sentence in
    // error, when a step failed.
    bool open(const std::filesystem::path& file, std::string& error) {
        // COM has to be started on this thread before Media Foundation. The thread is
        // the decoding thread of the player and has no window: "multithreaded" is the
        // mode for such a thread. A success has to be answered with CoUninitialize.
        m_comStarted = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        HRESULT result = MFStartup(MF_VERSION);
        if (FAILED(result)) {
            error = failedStep("Media Foundation cannot be started", result);
            return false;
        }
        m_mediaFoundationStarted = true;

        // The decoder of a video delivers YUV pictures (one brightness and two colour
        // differences per pixel, the colour differences at half the resolution). This
        // attribute lets the reader convert them to the RGB format asked for below.
        ComPtr<IMFAttributes> attributes;
        result = MFCreateAttributes(&attributes, 1);
        if (SUCCEEDED(result)) {
            result = attributes->SetUINT32(MF_SOURCE_READER_ENABLE_ADVANCED_VIDEO_PROCESSING, TRUE);
        }
        if (FAILED(result)) {
            error = failedStep("The settings of the video reader cannot be created", result);
            return false;
        }

        // path::c_str() is a wide string on Windows, so a path with letters outside of
        // ASCII works. This call fails for a file that is not a video.
        result = MFCreateSourceReaderFromURL(file.c_str(), attributes.Get(), &m_reader);
        if (FAILED(result)) {
            error = failedStep("Media Foundation cannot read the file as a video", result);
            return false;
        }

        // Only the video stream is read. A sound track, if the file has one, is not
        // even decoded.
        m_reader->SetStreamSelection(ALL_STREAMS, FALSE);
        m_reader->SetStreamSelection(VIDEO_STREAM, TRUE);

        // The output format: 32 bits per pixel, blue green red and an unused byte. This
        // call fails when Windows has no decoder for the video in the file.
        ComPtr<IMFMediaType> wanted;
        result = MFCreateMediaType(&wanted);
        if (SUCCEEDED(result)) {
            wanted->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            wanted->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
            result = m_reader->SetCurrentMediaType(VIDEO_STREAM, nullptr, wanted.Get());
        }
        if (FAILED(result)) {
            error =
                failedStep("Windows has no decoder that turns this video into pictures", result);
            return false;
        }

        if (!readOutputFormat()) {
            error = "The size of the video cannot be read";
            return false;
        }
        return true;
    }

    VideoFormat format() const override { return m_format; }

    VideoRead readFrame(std::vector<unsigned char>& pixels, double& stampSeconds) override {
        // Usually one turn. A turn without a sample and without the end of the file is
        // a gap in the stream: the next call brings the frame.
        while (true) {
            DWORD flags = 0;
            LONGLONG stamp = 0;
            ComPtr<IMFSample> sample;
            // A blocking call: it returns when the next frame is decoded.
            const HRESULT result =
                m_reader->ReadSample(VIDEO_STREAM, 0, nullptr, &flags, &stamp, &sample);
            if (FAILED(result) || (flags & MF_SOURCE_READERF_ERROR) != 0) {
                return VideoRead::Failed;
            }
            if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
                return VideoRead::EndOfFile;
            }
            if ((flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) != 0) {
                // The decoder changed how it lays out its pictures. The size has to
                // stay: the player has one texture of one size.
                const VideoFormat before = m_format;
                if (!readOutputFormat() || m_format.width != before.width ||
                    m_format.height != before.height) {
                    return VideoRead::Failed;
                }
            }
            if (sample == nullptr) {
                continue;
            }
            if (!copyFrame(sample.Get(), pixels)) {
                return VideoRead::Failed;
            }
            stampSeconds = static_cast<double>(stamp) / STAMP_UNITS_PER_SECOND;
            return VideoRead::Frame;
        }
    }

    bool rewind() override {
        // The position is a time in units of 100 nanoseconds, wrapped in a PROPVARIANT
        // (a struct that can hold a value of many types): VT_I8 is a 64 bit integer.
        // GUID_NULL says that the number is such a time.
        PROPVARIANT start;
        PropVariantInit(&start);
        start.vt = VT_I8;
        start.hVal.QuadPart = 0;
        const HRESULT moved = m_reader->SetCurrentPosition(GUID_NULL, start);
        PropVariantClear(&start);
        return SUCCEEDED(moved);
    }

private:
    // Reads the size of a frame, the time one frame stays and the row layout from the
    // output format the reader has in use. False when the size is not a size.
    bool readOutputFormat() {
        ComPtr<IMFMediaType> current;
        if (FAILED(m_reader->GetCurrentMediaType(VIDEO_STREAM, &current))) {
            return false;
        }
        UINT32 width = 0;
        UINT32 height = 0;
        MFGetAttributeSize(current.Get(), MF_MT_FRAME_SIZE, &width, &height);
        m_format.width = static_cast<int>(width);
        m_format.height = static_cast<int>(height);

        // The frame rate is a fraction: 30 / 1, or 30000 / 1001 for the 29.97 of
        // television. One frame stays for the inverse of it.
        UINT32 rateNumerator = 0;
        UINT32 rateDenominator = 0;
        MFGetAttributeRatio(current.Get(), MF_MT_FRAME_RATE, &rateNumerator, &rateDenominator);
        if (rateNumerator > 0 && rateDenominator > 0) {
            m_format.frameSeconds =
                static_cast<double>(rateDenominator) / static_cast<double>(rateNumerator);
        }

        // The distance between two rows in the memory of a frame, in bytes. Negative
        // for a picture that is stored bottom row first. It is stored as an unsigned
        // number and meant as a signed one. Without it the rows are taken to follow
        // each other without padding, top row first.
        UINT32 stride = 0;
        if (SUCCEEDED(current->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride))) {
            m_defaultStride = static_cast<LONG>(static_cast<INT32>(stride));
        } else {
            m_defaultStride = static_cast<LONG>(m_format.width * VIDEO_BYTES_PER_PIXEL);
        }
        return m_format.width > 0 && m_format.height > 0;
    }

    // Copies the picture of a decoded sample into pixels, top row first. False when
    // the memory of the sample cannot be reached.
    bool copyFrame(IMFSample* sample, std::vector<unsigned char>& pixels) const {
        // A sample can hold its bytes in several buffers: this call gives one that
        // holds all of them.
        ComPtr<IMFMediaBuffer> buffer;
        if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) {
            return false;
        }
        const std::size_t rowBytes =
            static_cast<std::size_t>(m_format.width) * VIDEO_BYTES_PER_PIXEL;
        pixels.resize(rowBytes * static_cast<std::size_t>(m_format.height));

        // The better way to the bytes: a buffer that knows it holds a picture tells
        // where the top row is and how far apart the rows are (IMF2DBuffer). As() asks
        // the buffer whether it is one.
        ComPtr<IMF2DBuffer> picture;
        if (SUCCEEDED(buffer.As(&picture))) {
            BYTE* topRow = nullptr;
            LONG pitch = 0;
            if (FAILED(picture->Lock2D(&topRow, &pitch))) {
                return false;
            }
            copyRows(topRow, pitch, rowBytes, m_format.height, pixels);
            picture->Unlock2D();
            return true;
        }

        // The plain way: the bytes as one block, laid out as the output format said.
        // Lock gives the pointer to the bytes, Unlock ends its use.
        BYTE* bytes = nullptr;
        DWORD byteCount = 0;
        if (FAILED(buffer->Lock(&bytes, nullptr, &byteCount))) {
            return false;
        }
        const auto pitchBytes = static_cast<std::size_t>(std::abs(m_defaultStride));
        const auto rowCount = static_cast<std::size_t>(m_format.height);
        const bool complete = pitchBytes >= rowBytes && byteCount >= pitchBytes * rowCount;
        if (complete) {
            // Bottom row first: the top row of the picture is the last one in memory.
            const BYTE* topRow = m_defaultStride < 0 ? bytes + pitchBytes * (rowCount - 1) : bytes;
            copyRows(topRow, m_defaultStride, rowBytes, m_format.height, pixels);
        }
        buffer->Unlock();
        return complete;
    }

    // Reads the file and hands out one decoded frame per call.
    ComPtr<IMFSourceReader> m_reader;
    VideoFormat m_format;
    LONG m_defaultStride = 0;
    bool m_comStarted = false;
    bool m_mediaFoundationStarted = false;
};

} // namespace

std::unique_ptr<VideoDecoder> openVideoDecoder(const std::filesystem::path& file,
                                               std::string& error) {
    if (!mediaFoundationInstalled()) {
        error = "Media Foundation is not installed (a Windows N edition needs the Media "
                "Feature Pack)";
        return nullptr;
    }
    auto decoder = std::make_unique<MediaFoundationDecoder>();
    if (!decoder->open(file, error)) {
        return nullptr;
    }
    return decoder;
}

} // namespace video

#endif // _WIN32
