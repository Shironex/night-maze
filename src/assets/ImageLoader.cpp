// Image file loader: decodes a PNG (or another format stb_image knows) into plain pixels.
// See docs/modules/assets/images.md
#include "assets/ImageLoader.hpp"

#include "core/Log.hpp"
#include "core/Paths.hpp"

#include <stb_image.h>

#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <limits>
#include <utility>

namespace assets {

namespace {

// Asks stb_image to keep the channel count of the file instead of converting the pixels.
constexpr int KEEP_FILE_CHANNELS = 0;

// Reads a whole file into bytes, without changing anything. Returns false when the file
// cannot be opened.
bool readBinaryFile(const std::filesystem::path& path, std::vector<unsigned char>& bytes) {
    // The stream takes the path object itself, not a string made from it. On Windows the
    // path holds wide characters and the stream opens the file through them, so a letter
    // outside the local code page is not damaged. std::ios::binary switches off the
    // translation of line endings, which would corrupt image data on Windows.
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    // An istreambuf_iterator reads the file one char at a time, and an iterator made
    // without a stream marks the end of the file. assign copies everything between the
    // two into the vector.
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

// Stores the reason in error, logs it once and returns false, so that a failing branch of
// loadImage is the single line "return fail(...)".
bool fail(std::string& error, const std::string& message) {
    error = message;
    core::logError(error);
    return false;
}

} // namespace

bool loadImage(const std::filesystem::path& path, Image& image, std::string& error,
               RowOrder rowOrder) {
    std::vector<unsigned char> fileBytes;
    if (!readBinaryFile(path, fileBytes)) {
        return fail(error, "Image file cannot be opened: " + core::pathText(path));
    }
    if (fileBytes.empty()) {
        return fail(error, "Image file is empty: " + core::pathText(path));
    }
    // stb_image takes the size of the data as an int.
    if (fileBytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return fail(error, "Image file is too large: " + core::pathText(path));
    }

    // stb_image decodes from memory, so it never sees the file name and its own handling
    // of file names on Windows does not matter. It writes the size and the channel count
    // of the file into the three ints and returns a block of width * height * channels
    // bytes that it allocated, or nullptr when the data is not an image it can decode.
    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* decoded = stbi_load_from_memory(fileBytes.data(), static_cast<int>(fileBytes.size()),
                                             &width, &height, &channels, KEEP_FILE_CHANNELS);
    if (decoded == nullptr) {
        // stbi_failure_reason gives a short text such as "unknown image type".
        return fail(error, "Image file cannot be decoded: " + core::pathText(path) + " (" +
                               stbi_failure_reason() + ")");
    }

    // Sizes in bytes are computed as std::size_t: three ints multiplied as int could
    // overflow for a very large picture.
    const std::size_t rowSize =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
    const auto rowCount = static_cast<std::size_t>(height);

    // The result is built in a local object and handed over at the very end, so image is
    // changed only when everything worked.
    Image loaded;
    loaded.width = width;
    loaded.height = height;
    loaded.channels = channels;
    loaded.pixels.resize(rowSize * rowCount);

    // stb_image returns the rows the way image files store them: top row first. OpenGL
    // (and the OBJ format) put texture coordinate v = 0 of a 2D texture at the bottom,
    // so for RowOrder::BottomFirst the rows are copied in reverse order: the last row of
    // the file becomes the first row here. The pixels inside a row keep their order,
    // otherwise the picture would be mirrored. For RowOrder::TopFirst (the faces of
    // a cube map) the rows are copied as they are.
    for (std::size_t row = 0; row < rowCount; ++row) {
        const std::size_t sourceRow = rowOrder == RowOrder::BottomFirst ? rowCount - 1 - row : row;
        const stbi_uc* source = decoded + sourceRow * rowSize;
        std::copy_n(source, rowSize,
                    loaded.pixels.begin() + static_cast<std::ptrdiff_t>(row * rowSize));
    }

    // The block belongs to stb_image and must be released with its own function. The
    // pixels are already copied into the vector, which frees itself.
    stbi_image_free(decoded);

    // Moving hands the pixel vector over without copying its bytes a second time.
    image = std::move(loaded);
    error.clear();
    return true;
}

} // namespace assets
