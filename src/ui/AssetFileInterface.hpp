// AssetFileInterface: lets RmlUi read its files from the assets directory of the game.
#pragma once

#include <RmlUi/Core/FileInterface.h>

#include <cstddef>
#include <map>
#include <vector>

namespace ui {

/// The way RmlUi opens files in this program.
///
/// RmlUi never opens a file itself: for every document, style sheet, font and image it
/// asks a "file interface" with the five functions below, which look like the file
/// functions of C (fopen, fclose, fread, fseek, ftell). This one reads from the assets
/// directory next to the executable: the path RmlUi asks for, for example
/// "ui/main_menu.rml", is a path inside that directory (core::assetPath).
///
/// Open reads the whole file into memory at once (core::readBinaryFile), and the other
/// functions work on those bytes. The files of a menu are small, and the code stays
/// simple: a position that moves through a vector.
///
/// Why not the default of RmlUi: it opens files by a narrow string and relative to the
/// working directory. This one works from any working directory and for an install
/// path with letters outside of ASCII on Windows.
class AssetFileInterface final : public Rml::FileInterface {
public:
    /// Opens a file of the assets directory. path is UTF-8 text with forward slashes.
    /// Returns a handle, a number that names the open file in the other functions, or
    /// 0 when the file cannot be opened.
    Rml::FileHandle Open(const Rml::String& path) override;

    /// Closes the file: its bytes are released.
    void Close(Rml::FileHandle file) override;

    /// Copies up to size bytes from the current position into buffer and moves the
    /// position on. Returns how many bytes were copied: fewer than size at the end.
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override;

    /// Moves the position. origin is SEEK_SET (from the start), SEEK_CUR (from the
    /// current position) or SEEK_END (from the end), as for fseek. False when the new
    /// position would be outside of the file.
    bool Seek(Rml::FileHandle file, long offset, int origin) override;

    /// The current position, in bytes from the start.
    size_t Tell(Rml::FileHandle file) override;

private:
    // One open file: all of its bytes and the place the next Read starts at.
    struct OpenFile {
        std::vector<unsigned char> bytes;
        std::size_t position = 0;
    };

    // The open files by their handle. Handles are counted up from 1, because 0 means
    // "could not be opened".
    std::map<Rml::FileHandle, OpenFile> m_files;
    Rml::FileHandle m_nextHandle = 1;
};

} // namespace ui
