// Paths of files that ship with the program, found relative to the executable.
// See docs/modules/core/paths.md
#pragma once

#include <filesystem>

namespace core {

/// Absolute path of the directory that contains the running executable.
/// Throws std::runtime_error if the operating system cannot report it.
std::filesystem::path executableDir();

/// Path of a file in the assets directory that lies next to the executable, for example
/// assetPath("shaders/triangle.vert"). It does not check that the file exists: the code
/// that opens the file reports that.
std::filesystem::path assetPath(const std::filesystem::path& relativePath);

} // namespace core
