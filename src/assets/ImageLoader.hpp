// Image file loader: decodes a PNG (or another format stb_image knows) into plain pixels.
// See docs/modules/assets/images.md
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace assets {

/// A picture in the memory of the program: plain data, no OpenGL.
///
/// The pixels are stored row by row without gaps. One pixel is channels bytes (red, green,
/// blue and, with 4 channels, alpha), each from 0 to 255. The first row in pixels is the
/// BOTTOM row of the picture, because that is the order OpenGL expects: texture coordinate
/// v = 0 is the bottom edge.
struct Image {
    /// Width in pixels.
    int width = 0;
    /// Height in pixels.
    int height = 0;
    /// Bytes per pixel, as stored in the file: 1 (grey), 2 (grey and alpha), 3 (RGB) or
    /// 4 (RGBA).
    int channels = 0;
    /// width * height * channels bytes, bottom row first.
    std::vector<unsigned char> pixels;
};

/// Reads an image file and decodes it into pixels. The channel count of the file is kept.
/// The path may contain any characters, also non ASCII ones on Windows.
///
/// Returns true and fills image on success. On failure (the file cannot be opened, it is
/// empty, it is not an image the decoder knows) it logs the error once, puts the same text
/// into error, leaves image unchanged and returns false. It does not throw.
bool loadImage(const std::filesystem::path& path, Image& image, std::string& error);

} // namespace assets
