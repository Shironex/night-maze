// VideoDecoder on macOS: frames of a video file through AVFoundation.
//
// UNTESTED. This file was written from the documentation of AVFoundation on a Windows
// computer: it has never been compiled and never been run. The first build on a Mac has
// to check it (the list is in the hand-over notes of the menu video). Until then every
// line here is a careful guess. When it does not work the game is still usable: a failed
// openVideoDecoder makes the main menu show its still picture instead of the video.
//
// The file is Objective-C++ (the ending .mm): C++ that may also use the classes of
// Apple, whose methods are called with square brackets, [object method:argument]. It is
// compiled with automatic reference counting (the switch -fobjc-arc in CMakeLists.txt),
// so the Objective-C objects held by the class below are released by the compiler when
// the class lets go of them.
#include "video/VideoDecoder.hpp"

#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <Foundation/Foundation.h>

#include <cstring>

namespace video {

namespace {

// The text of an error of AVFoundation, or a general sentence when there is none.
std::string errorText(NSError* problem, const char* otherwise) {
    if (problem != nil && problem.localizedDescription != nil) {
        return std::string(problem.localizedDescription.UTF8String);
    }
    return otherwise;
}

// AVFoundation reads a file through three objects. The asset (AVAsset) is the file as
// a whole, with its tracks. The reader (AVAssetReader) reads an asset from its start to
// its end, once. The output (AVAssetReaderTrackOutput) belongs to a reader and hands
// out the decoded frames of one track, one per call.
class AvFoundationDecoder final : public VideoDecoder {
public:
    AvFoundationDecoder() = default;

    ~AvFoundationDecoder() override {
        // A thread started with std::thread has no autorelease pool of its own: the
        // temporary objects of the calls below are released at the end of this block.
        @autoreleasepool {
            // A reader that is still reading keeps threads of its own busy.
            [m_reader cancelReading];
            m_output = nil;
            m_reader = nil;
            m_track = nil;
            m_asset = nil;
        }
    }

    // Opens the file and starts reading it. False, with a sentence in error, when
    // a step failed.
    bool open(const std::filesystem::path& file, std::string& error) {
        @autoreleasepool {
            // The path of the file as a URL. path::c_str() is UTF-8 text on macOS.
            NSString* pathText = [NSString stringWithUTF8String:file.c_str()];
            if (pathText == nil) {
                error = "The path of the video is not valid text";
                return false;
            }
            m_asset = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:pathText] options:nil];

// The three properties read here load the file when they are asked and block until
// that is done. Apple marks them as deprecated since macOS 15 in favour of calls that
// answer later, on another thread. This code already runs on a thread of its own whose
// only job is to wait for the decoder, so the blocking calls are the right ones here,
// and the warning about them is switched off for these lines.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            NSArray<AVAssetTrack*>* tracks = [m_asset tracksWithMediaType:AVMediaTypeVideo];
            if (tracks.count == 0) {
                error = "AVFoundation finds no video track in the file";
                return false;
            }
            m_track = tracks.firstObject;

            // The size of the stored pictures, from the description of the track's
            // format. It is the size of the pixel buffers the output will hand out.
            NSArray* descriptions = m_track.formatDescriptions;
            if (descriptions.count == 0) {
                error = "The video track does not describe its format";
                return false;
            }
            const auto description = (__bridge CMVideoFormatDescriptionRef)descriptions.firstObject;
            const CMVideoDimensions size = CMVideoFormatDescriptionGetDimensions(description);
            m_format.width = static_cast<int>(size.width);
            m_format.height = static_cast<int>(size.height);

            // Frames per second, as the track states it. One frame stays for the
            // inverse of it.
            const float framesPerSecond = m_track.nominalFrameRate;
#pragma clang diagnostic pop
            if (framesPerSecond > 0.0F) {
                m_format.frameSeconds = 1.0 / static_cast<double>(framesPerSecond);
            }
            if (m_format.width < 1 || m_format.height < 1) {
                error = "The size of the video cannot be read";
                return false;
            }
            return startReader(error);
        }
    }

    VideoFormat format() const override { return m_format; }

    VideoRead readFrame(std::vector<unsigned char>& pixels, double& stampSeconds) override {
        @autoreleasepool {
            if (m_output == nil) {
                return VideoRead::Failed;
            }
            // Usually one turn. A sample without a picture is a marker in the stream:
            // the next call brings the frame.
            while (true) {
                // A blocking call: it returns when the next frame is decoded. "copy" in
                // the name means that the caller owns the result and has to release it
                // with CFRelease: it is a C object, which the automatic reference
                // counting does not look after.
                const CMSampleBufferRef sample = [m_output copyNextSampleBuffer];
                if (sample == nullptr) {
                    // No sample: the end of the file when the reader says it has
                    // completed, an error otherwise.
                    return m_reader.status == AVAssetReaderStatusCompleted ? VideoRead::EndOfFile
                                                                           : VideoRead::Failed;
                }
                const CVImageBufferRef picture = CMSampleBufferGetImageBuffer(sample);
                if (picture == nullptr) {
                    CFRelease(sample);
                    continue;
                }
                const bool copied = copyPicture(picture, pixels);
                // The time the file gives the frame, as a fraction, turned into seconds.
                stampSeconds = CMTimeGetSeconds(CMSampleBufferGetPresentationTimeStamp(sample));
                CFRelease(sample);
                return copied ? VideoRead::Frame : VideoRead::Failed;
            }
        }
    }

    bool rewind() override {
        @autoreleasepool {
            // A reader reads its asset once and cannot go back. Going back to the
            // first frame is a new reader for the same asset.
            [m_reader cancelReading];
            m_output = nil;
            m_reader = nil;
            std::string ignored;
            return startReader(ignored);
        }
    }

private:
    // Creates a reader for the asset, with an output that decodes the video track into
    // plain pixels, and starts it. False, with a sentence in error, when a step failed.
    bool startReader(std::string& error) {
        NSError* problem = nil;
        AVAssetReader* reader = [[AVAssetReader alloc] initWithAsset:m_asset error:&problem];
        if (reader == nil) {
            error = errorText(problem, "AVFoundation cannot read the file");
            return false;
        }

        // The format of the pixels the output hands out: 32 bits per pixel, blue green
        // red and one more byte. Without these settings it would hand out the
        // compressed data of the file, not pictures.
        NSDictionary* settings =
            @{(NSString*)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};
        AVAssetReaderTrackOutput* output =
            [[AVAssetReaderTrackOutput alloc] initWithTrack:m_track outputSettings:settings];
        // The pixels are copied out of every sample right away (copyPicture), so the
        // output does not have to make a copy of its own first.
        output.alwaysCopiesSampleData = NO;
        if (![reader canAddOutput:output]) {
            error = "AVFoundation cannot decode the video track into pixels";
            return false;
        }
        [reader addOutput:output];
        if (![reader startReading]) {
            error = errorText(reader.error, "AVFoundation cannot start reading the file");
            return false;
        }
        m_reader = reader;
        m_output = output;
        return true;
    }

    // Copies a decoded picture into pixels, top row first. False when the picture is
    // not what open announced.
    bool copyPicture(CVImageBufferRef picture, std::vector<unsigned char>& pixels) const {
        // The memory of a pixel buffer may be read only between these two calls.
        CVPixelBufferLockBaseAddress(picture, kCVPixelBufferLock_ReadOnly);
        const auto* topRow =
            static_cast<const unsigned char*>(CVPixelBufferGetBaseAddress(picture));
        // The distance from one row to the next, in bytes. It can be larger than a row:
        // the rows are padded to a size the graphics hardware likes.
        const std::size_t pitch = CVPixelBufferGetBytesPerRow(picture);
        const std::size_t rowBytes =
            static_cast<std::size_t>(m_format.width) * VIDEO_BYTES_PER_PIXEL;
        const auto rowCount = static_cast<std::size_t>(m_format.height);
        const bool asAnnounced =
            topRow != nullptr && pitch >= rowBytes &&
            CVPixelBufferGetWidth(picture) == static_cast<std::size_t>(m_format.width) &&
            CVPixelBufferGetHeight(picture) == rowCount;
        if (asAnnounced) {
            pixels.resize(rowBytes * rowCount);
            for (std::size_t row = 0; row < rowCount; ++row) {
                std::memcpy(pixels.data() + rowBytes * row, topRow + pitch * row, rowBytes);
            }
        }
        CVPixelBufferUnlockBaseAddress(picture, kCVPixelBufferLock_ReadOnly);
        return asAnnounced;
    }

    AVAsset* m_asset = nil;
    AVAssetTrack* m_track = nil;
    AVAssetReader* m_reader = nil;
    AVAssetReaderTrackOutput* m_output = nil;
    VideoFormat m_format;
};

} // namespace

std::unique_ptr<VideoDecoder> openVideoDecoder(const std::filesystem::path& file,
                                               std::string& error) {
    auto decoder = std::make_unique<AvFoundationDecoder>();
    if (!decoder->open(file, error)) {
        return nullptr;
    }
    return decoder;
}

} // namespace video
