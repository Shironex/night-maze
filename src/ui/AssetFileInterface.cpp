// AssetFileInterface: lets RmlUi read its files from the assets directory of the game.
#include "ui/AssetFileInterface.hpp"

#include "core/Files.hpp"
#include "core/Paths.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <utility>

namespace ui {

Rml::FileHandle AssetFileInterface::Open(const Rml::String& path) {
    // RmlUi hands over UTF-8 text. A std::filesystem::path built from a plain
    // std::string would read it in the code page of Windows, so the text goes in as
    // a std::u8string, which is always taken as UTF-8.
    const std::filesystem::path relativePath(std::u8string(path.begin(), path.end()));

    OpenFile file;
    if (!core::readBinaryFile(core::assetPath(relativePath), file.bytes)) {
        // 0: RmlUi then logs which file it could not open.
        return 0;
    }

    const Rml::FileHandle handle = m_nextHandle;
    ++m_nextHandle;
    m_files.emplace(handle, std::move(file));
    return handle;
}

void AssetFileInterface::Close(Rml::FileHandle file) {
    m_files.erase(file);
}

size_t AssetFileInterface::Read(void* buffer, size_t size, Rml::FileHandle file) {
    const auto found = m_files.find(file);
    if (found == m_files.end()) {
        return 0;
    }
    OpenFile& open = found->second;

    // Not more than what is left between the position and the end.
    const std::size_t count = std::min(size, open.bytes.size() - open.position);
    if (count > 0) {
        std::memcpy(buffer, open.bytes.data() + open.position, count);
        open.position += count;
    }
    return count;
}

bool AssetFileInterface::Seek(Rml::FileHandle file, long offset, int origin) {
    const auto found = m_files.find(file);
    if (found == m_files.end()) {
        return false;
    }
    OpenFile& open = found->second;

    // The place the offset is counted from. The numbers are signed here, because an
    // offset from the current position or from the end can be negative.
    const auto size = static_cast<long long>(open.bytes.size());
    long long base = 0;
    if (origin == SEEK_CUR) {
        base = static_cast<long long>(open.position);
    } else if (origin == SEEK_END) {
        base = size;
    } else if (origin != SEEK_SET) {
        return false;
    }

    const long long target = base + offset;
    if (target < 0 || target > size) {
        return false;
    }
    open.position = static_cast<std::size_t>(target);
    return true;
}

size_t AssetFileInterface::Tell(Rml::FileHandle file) {
    const auto found = m_files.find(file);
    if (found == m_files.end()) {
        return 0;
    }
    return found->second.position;
}

} // namespace ui
