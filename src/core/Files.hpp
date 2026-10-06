// Reading whole files into memory, and the name of the one font file of the program.
// See docs/modules/core/paths.md
#pragma once

#include <filesystem>
#include <vector>

namespace core {

/// The font of the program, relative to the assets directory (core::assetPath). The
/// debug panels and the menu both draw their text with it. Licence: assets/fonts/OFL.txt.
constexpr const char* TEXT_FONT_FILE = "fonts/AtkinsonHyperlegible-Regular.ttf";

/// Reads a whole file into bytes. Returns false when the file cannot be opened, and
/// leaves bytes as it was then. It logs nothing: the caller knows what the file is for
/// and writes the message.
bool readBinaryFile(const std::filesystem::path& path, std::vector<unsigned char>& bytes);

} // namespace core
