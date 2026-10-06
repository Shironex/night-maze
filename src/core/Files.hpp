// Reading and writing whole files, and the name of the one font file of the program.
// See docs/modules/core/paths.md
#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace core {

/// The font of the program, relative to the assets directory (core::assetPath). The
/// debug panels and the menu both draw their text with it. Licence: assets/fonts/OFL.txt.
constexpr const char* TEXT_FONT_FILE = "fonts/AtkinsonHyperlegible-Regular.ttf";

/// Reads a whole file into bytes. Returns false when the file cannot be opened, and
/// leaves bytes as it was then. It logs nothing: the caller knows what the file is for
/// and writes the message.
bool readBinaryFile(const std::filesystem::path& path, std::vector<unsigned char>& bytes);

/// Reads a whole file as text: its bytes as they are, with no change to the line ends.
/// Returns false when the file cannot be opened, and leaves text as it was then. Like
/// readBinaryFile it logs nothing.
bool readTextFile(const std::filesystem::path& path, std::string& text);

/// Writes text into a file, in place of what the file held before. The bytes are written
/// as they are. Returns false when the file cannot be opened for writing or not all of
/// the text could be written. It logs nothing.
bool writeTextFile(const std::filesystem::path& path, std::string_view text);

} // namespace core
