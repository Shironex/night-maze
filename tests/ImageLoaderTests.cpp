// Tests of assets::loadImage.
// See docs/modules/assets/images.md
#include "assets/ImageLoader.hpp"

#include <doctest/doctest.h>

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

    CHECK_FALSE(assets::loadImage(assetsDirectory() / "shaders" / "basic.vert", image, error));
    CHECK(error.find("cannot be decoded") != std::string::npos);
    CHECK(error.find("basic.vert") != std::string::npos);
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
