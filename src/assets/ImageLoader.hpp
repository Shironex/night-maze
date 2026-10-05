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
/// blue and, with 4 channels, alpha), each from 0 to 255. Which row comes first is chosen
/// when the picture is loaded (RowOrder). Unless asked otherwise, the first row in pixels
/// is the BOTTOM row of the picture, because that is the order OpenGL expects for a 2D
/// texture: texture coordinate v = 0 is the bottom edge.
struct Image {
    /// Width in pixels.
    int width = 0;
    /// Height in pixels.
    int height = 0;
    /// Bytes per pixel, as stored in the file: 1 (grey), 2 (grey and alpha), 3 (RGB) or
    /// 4 (RGBA).
    int channels = 0;
    /// width * height * channels bytes, bottom row first (or top row first when the
    /// picture was loaded with RowOrder::TopFirst).
    std::vector<unsigned char> pixels;
};

/// Which row of the picture comes first in Image::pixels.
enum class RowOrder {
    /// The bottom row first: the rows of the file in reverse order. For 2D textures,
    /// where OpenGL takes the first row it is given as texture coordinate v = 0, the
    /// bottom edge.
    BottomFirst,
    /// The top row first: the rows as they are in the file, no flip. For the faces of
    /// a cube map. A cube map is not read with (u, v) but with a direction, and its
    /// rules say that on each face the coordinate t = 0 is the TOP of the picture (the
    /// convention comes from RenderMan, where the origin of a picture is its top left
    /// corner). OpenGL still takes the first row it is given as t = 0, so for a face
    /// that row has to be the top one. A face loaded bottom row first would be shown
    /// upside down, and it would not fit its neighbours.
    TopFirst,
};

/// Reads an image file and decodes it into pixels. The channel count of the file is kept.
/// The path may contain any characters, also non ASCII ones on Windows. rowOrder says
/// which row of the picture comes first in the result: the bottom one unless asked
/// otherwise, which is right for every 2D texture.
///
/// Returns true and fills image on success. On failure (the file cannot be opened, it is
/// empty, it is not an image the decoder knows) it logs the error once, puts the same text
/// into error, leaves image unchanged and returns false. It does not throw.
bool loadImage(const std::filesystem::path& path, Image& image, std::string& error,
               RowOrder rowOrder = RowOrder::BottomFirst);

} // namespace assets
