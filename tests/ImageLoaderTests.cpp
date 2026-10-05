// Tests of assets::loadImage.
// See docs/modules/assets/images.md
#include "assets/ImageLoader.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

// The assets directory of the repository. CMake passes its absolute path to the test
// program as the macro NIGHT_MAZE_ASSETS_DIR (target_compile_definitions in CMakeLists.txt),
// so the tests find the real textures whatever the working directory is. The macro is a
// string literal, and a path can be made from one without writing the type again.
std::filesystem::path assetsDirectory() {
    return NIGHT_MAZE_ASSETS_DIR;
}

// A tiny test picture, 2 pixels wide and 3 pixels high, 3 bytes (red, green, blue) per
// pixel, in the order of an image file: TOP row first. Every pixel is different, so a
// wrong row order and a mirrored row both show up. One row is 6 bytes, which is not
// a multiple of 4.
constexpr int PICTURE_WIDTH = 2;
constexpr int PICTURE_HEIGHT = 3;
constexpr int PICTURE_CHANNELS = 3;
constexpr std::size_t PICTURE_BYTE_COUNT = 18;
constexpr std::array<unsigned char, PICTURE_BYTE_COUNT> PICTURE_TOP_ROW_FIRST = {
    255, 0,  0,   0,   255, 0,  // top row: red, green
    0,   0,  255, 255, 255, 0,  // middle row: blue, yellow
    10,  20, 30,  40,  50,  60, // bottom row: two dark colors
};

// Writes the test picture as a binary PPM file ("P6"). PPM is the simplest image format
// stb_image reads: a short text header (magic number, width, height, largest value) and
// then the raw bytes of the pixels, top row first. No compression, so the test can write
// it by hand.
void writeTestPicture(const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::binary);
    file << "P6\n" << PICTURE_WIDTH << " " << PICTURE_HEIGHT << "\n255\n";
    for (const unsigned char byte : PICTURE_TOP_ROW_FIRST) {
        file.put(static_cast<char>(byte));
    }
}

// The average value (0 to 255) of one channel over a rectangle of an image: columnCount
// columns starting at firstColumn, rowCount rows starting at firstRow. Row 0 is the first
// row in memory, which is the bottom row of the picture.
double channelAverage(const assets::Image& image, int channel, int firstColumn, int firstRow,
                      int columnCount, int rowCount) {
    double sum = 0.0;
    for (int row = firstRow; row < firstRow + rowCount; ++row) {
        for (int column = firstColumn; column < firstColumn + columnCount; ++column) {
            const std::size_t pixel =
                static_cast<std::size_t>(row) * static_cast<std::size_t>(image.width) +
                static_cast<std::size_t>(column);
            sum += image.pixels[pixel * static_cast<std::size_t>(image.channels) +
                                static_cast<std::size_t>(channel)];
        }
    }
    return sum / (static_cast<double>(columnCount) * static_cast<double>(rowCount));
}

} // namespace

TEST_CASE("the stone textures of the game load with the size and channels they were made with") {
    // The loop variable is a std::string and not a const char*, so that CAPTURE prints the
    // file name itself when a check fails.
    for (const std::string fileName : {"wall_stone.png", "floor_stone.png"}) {
        CAPTURE(fileName);
        assets::Image image;
        std::string error;

        CHECK(assets::loadImage(assetsDirectory() / "textures" / fileName, image, error));
        CHECK(error.empty());
        CHECK(image.width == 512);
        CHECK(image.height == 512);
        CHECK(image.channels == 3);
        // 512 * 512 pixels with 3 bytes each.
        CHECK(image.pixels.size() == 786432U);
    }
}

TEST_CASE("the normal maps of the game load, and most of their texels are flat") {
    for (const std::string fileName : {"wall_stone_normal.png", "floor_stone_normal.png"}) {
        CAPTURE(fileName);
        assets::Image image;
        std::string error;

        REQUIRE(assets::loadImage(assetsDirectory() / "textures" / fileName, image, error));
        CHECK(error.empty());
        CHECK(image.width == 512);
        CHECK(image.height == 512);
        CHECK(image.channels == 3);

        // A flat texel is the direction (0, 0, 1), stored as (128, 128, 255). The faces
        // of the stones are nearly flat and the two slopes of every joint lean in
        // opposite directions, so the average of the whole picture is close to that
        // colour. An average far from it would tilt the light on every wall.
        const std::array<double, 3> average = {
            channelAverage(image, 0, 0, 0, image.width, image.height),
            channelAverage(image, 1, 0, 0, image.width, image.height),
            channelAverage(image, 2, 0, 0, image.width, image.height),
        };
        CHECK(average[0] == doctest::Approx(128.0).epsilon(0.02));
        CHECK(average[1] == doctest::Approx(128.0).epsilon(0.02));
        CHECK(average[2] > 245.0);
    }
}

TEST_CASE("the wall normal map follows the OpenGL convention: a joint is a groove") {
    assets::Image image;
    std::string error;
    REQUIRE(
        assets::loadImage(assetsDirectory() / "textures" / "wall_stone_normal.png", image, error));

    // The loader returns the bottom row first, so row 0 is v = 0 and rows count upwards,
    // like v. The bottom row of blocks covers the rows 0 to 63 and its first block the
    // columns 0 to 127. Each side of a block has 3 pixels of joint and then 5 pixels of
    // bevel that rise to the face (tools/blender/make_textures.py).
    //
    // In a groove the bevel BELOW the joint is the top edge of a block: it faces up, so
    // its normal has a positive y, a green above 128. The bevel ABOVE the joint is the
    // bottom edge of the next block: it faces down, green below 128. A map in the
    // DirectX convention, or a ridge instead of a groove, would have the two swapped.
    constexpr int BLOCK_MIDDLE_FIRST_COLUMN = 24;
    constexpr int BLOCK_MIDDLE_COLUMN_COUNT = 80;
    constexpr int TOP_BEVEL_ROW = 58;   // below the joint between the rows 63 and 64
    constexpr int BOTTOM_BEVEL_ROW = 5; // above the joint at the bottom of the picture
    const double greenBelowJoint = channelAverage(image, 1, BLOCK_MIDDLE_FIRST_COLUMN,
                                                  TOP_BEVEL_ROW, BLOCK_MIDDLE_COLUMN_COUNT, 1);
    const double greenAboveJoint = channelAverage(image, 1, BLOCK_MIDDLE_FIRST_COLUMN,
                                                  BOTTOM_BEVEL_ROW, BLOCK_MIDDLE_COLUMN_COUNT, 1);
    CHECK(greenBelowJoint > 150.0);
    CHECK(greenAboveJoint < 106.0);

    // The same along x, in the red channel: the bevel left of a vertical joint is the
    // right edge of a block and faces right (red above 128), the bevel right of the
    // joint faces left (red below 128).
    constexpr int BLOCK_MIDDLE_FIRST_ROW = 16;
    constexpr int BLOCK_MIDDLE_ROW_COUNT = 32;
    constexpr int RIGHT_BEVEL_COLUMN = 122; // left of the joint between the columns 127 and 128
    constexpr int LEFT_BEVEL_COLUMN = 5;    // right of the joint at the left edge of the picture
    const double redLeftOfJoint = channelAverage(image, 0, RIGHT_BEVEL_COLUMN,
                                                 BLOCK_MIDDLE_FIRST_ROW, 1, BLOCK_MIDDLE_ROW_COUNT);
    const double redRightOfJoint = channelAverage(
        image, 0, LEFT_BEVEL_COLUMN, BLOCK_MIDDLE_FIRST_ROW, 1, BLOCK_MIDDLE_ROW_COUNT);
    CHECK(redLeftOfJoint > 150.0);
    CHECK(redRightOfJoint < 106.0);

    // Every texel points out of the surface: its z is positive, a blue above 128. The
    // blue byte is the third of every pixel.
    unsigned char lowestBlue = 255;
    for (std::size_t i = 2; i < image.pixels.size(); i += 3) {
        lowestBlue = std::min(lowestBlue, image.pixels[i]);
    }
    CHECK(lowestBlue > 128);
}

TEST_CASE("the rows are flipped: the first row in memory is the bottom row of the file") {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "night_maze_flip_test.ppm";
    writeTestPicture(path);

    assets::Image image;
    std::string error;
    const bool loaded = assets::loadImage(path, image, error);
    std::filesystem::remove(path);

    CHECK(loaded);
    CHECK(image.width == PICTURE_WIDTH);
    CHECK(image.height == PICTURE_HEIGHT);
    CHECK(image.channels == PICTURE_CHANNELS);

    // The same pixels with the three rows in reverse order. Inside a row nothing moves:
    // the left pixel stays on the left.
    const std::vector<unsigned char> bottomRowFirst = {
        10,  20, 30,  40,  50,  60, // bottom row of the file
        0,   0,  255, 255, 255, 0,  // middle row
        255, 0,  0,   0,   255, 0,  // top row of the file
    };
    CHECK(image.pixels == bottomRowFirst);
}

TEST_CASE("with RowOrder::TopFirst the rows are not flipped: they stay as in the file") {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "night_maze_no_flip_test.ppm";
    writeTestPicture(path);

    assets::Image image;
    std::string error;
    const bool loaded = assets::loadImage(path, image, error, assets::RowOrder::TopFirst);
    std::filesystem::remove(path);

    CHECK(loaded);
    CHECK(error.empty());
    CHECK(image.width == PICTURE_WIDTH);
    CHECK(image.height == PICTURE_HEIGHT);
    CHECK(image.channels == PICTURE_CHANNELS);

    // Exactly the bytes that were written to the file: the top row first. This is the
    // order the faces of a cube map are loaded in.
    const std::vector<unsigned char> topRowFirst(PICTURE_TOP_ROW_FIRST.begin(),
                                                 PICTURE_TOP_ROW_FIRST.end());
    CHECK(image.pixels == topRowFirst);
}

TEST_CASE("a path with letters outside ASCII can be loaded") {
    // The Polish letters are written as universal character names (a backslash, the letter
    // u and the code of the character) in a UTF-8 literal, so the test does not depend on
    // the encoding the compiler assumes for this source file. The name is "zolty" with the
    // Polish letters (z with a dot, o with an acute accent, l with a stroke) and then one
    // Japanese character, which no European code page of Windows contains.
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        std::filesystem::path(u8"night_maze_\u017C\u00F3\u0142ty_\u65E5.ppm");
    writeTestPicture(path);

    assets::Image image;
    std::string error;
    const bool loaded = assets::loadImage(path, image, error);
    std::filesystem::remove(path);

    CHECK(loaded);
    CHECK(error.empty());
    CHECK(image.width == PICTURE_WIDTH);
    CHECK(image.height == PICTURE_HEIGHT);
    CHECK(image.pixels.size() == PICTURE_BYTE_COUNT);
}

TEST_CASE("a missing file is reported and leaves the image unchanged") {
    // An image that already holds something, to see that a failed load does not touch it.
    assets::Image image;
    image.width = 7;
    image.pixels = {1, 2, 3};
    std::string error;

    CHECK_FALSE(
        assets::loadImage(assetsDirectory() / "textures" / "no_such_file.png", image, error));
    CHECK(error.find("cannot be opened") != std::string::npos);
    CHECK(error.find("no_such_file.png") != std::string::npos);
    CHECK(image.width == 7);
    CHECK(image.pixels.size() == 3U);
}

TEST_CASE("a file that is not an image is reported") {
    // A shader is a text file: it opens fine, but no image decoder accepts it.
    assets::Image image;
    std::string error;

    CHECK_FALSE(assets::loadImage(assetsDirectory() / "shaders" / "color.vert", image, error));
    CHECK(error.find("cannot be decoded") != std::string::npos);
    CHECK(error.find("color.vert") != std::string::npos);
    CHECK(image.pixels.empty());
}

TEST_CASE("an empty file is reported") {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "night_maze_empty_test.png";
    {
        // Opening a file for writing creates it. The braces close it again right away.
        const std::ofstream file(path, std::ios::binary);
    }

    assets::Image image;
    std::string error;
    const bool loaded = assets::loadImage(path, image, error);
    std::filesystem::remove(path);

    CHECK_FALSE(loaded);
    CHECK(error.find("is empty") != std::string::npos);
}

TEST_CASE("a successful load clears the error text of an earlier failure") {
    assets::Image image;
    std::string error = "left over from an earlier call";

    CHECK(assets::loadImage(assetsDirectory() / "textures" / "wall_stone.png", image, error));
    CHECK(error.empty());
}
