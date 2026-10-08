// Reading and writing whole files, and the name of the one font file of the program.
#include "core/Files.hpp"

#include <fstream>
#include <iterator>

namespace core {

bool readBinaryFile(const std::filesystem::path& path, std::vector<unsigned char>& bytes) {
    // The stream takes the path object itself, so on Windows a letter outside the local
    // code page is not damaged (same reason as in assets/ImageLoader.cpp).
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool readTextFile(const std::filesystem::path& path, std::string& text) {
    // Binary: the bytes arrive as they are in the file. In text mode Windows would
    // turn its line ends into single characters and macOS would not, and whoever reads
    // the text would see two different things.
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool writeTextFile(const std::filesystem::path& path, std::string_view text) {
    // trunc: an existing file is emptied first, so nothing of the old text is left
    // behind a shorter new one.
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    file.write(text.data(), static_cast<std::streamsize>(text.size()));
    // good() is false when the write failed, for example on a full disk.
    return file.good();
}

} // namespace core
