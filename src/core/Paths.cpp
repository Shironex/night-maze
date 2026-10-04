// Paths of files that ship with the program, found relative to the executable.
// See docs/modules/core/paths.md
#include "core/Paths.hpp"

#include <stdexcept>
#include <string>

// The C++ standard library cannot tell where the executable is, so each operating system
// needs its own call. This is the only platform specific code in the file.
#if defined(__APPLE__)
#include <mach-o/dyld.h>

#include <cstdint>
#elif defined(_WIN32)
// Keep windows.h small, and stop it from defining the min and max macros, which break
// std::min and std::max. The header is included only here, never in a .hpp file.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#error "core/Paths.cpp supports macOS and Windows only"
#endif

namespace core {

namespace {

// Name of the directory with shaders, models and textures, next to the executable.
constexpr const char* ASSETS_DIRECTORY = "assets";

#if defined(__APPLE__)

// Full path of the executable file on macOS.
std::filesystem::path executableFile() {
    // The first call has no buffer (size 0), so it fails on purpose and writes the size
    // it needs, including the terminating zero, into size.
    std::uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);

    // The second call gets a buffer of exactly that size and returns 0 on success.
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) != 0) {
        throw std::runtime_error("Failed to read the executable path (_NSGetExecutablePath)");
    }

    // The reported path may go through a symbolic link or contain ".." parts. canonical
    // resolves both and returns the absolute path of the real file. c_str() stops at the
    // terminating zero that the system wrote into the buffer.
    return std::filesystem::canonical(buffer.c_str());
}

#elif defined(_WIN32)

// Longest path Windows can report, in wide characters, including the terminating zero.
// MAX_PATH (260) is not a hard limit on current Windows, so one buffer of the documented
// maximum is used instead of growing a small buffer in a loop.
constexpr DWORD MAX_LONG_PATH_LENGTH = 32768;

// Full path of the executable file on Windows.
std::filesystem::path executableFile() {
    // Wide characters (UTF-16), so a user name with non ASCII letters is not damaged.
    // The buffer lives on the heap: 32768 wide characters are 64 KB, too much for the stack.
    std::wstring buffer(MAX_LONG_PATH_LENGTH, L'\0');

    // nullptr as the module means the executable of the current process. The function
    // returns the number of characters written, without the terminating zero.
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), MAX_LONG_PATH_LENGTH);
    if (length == 0) {
        throw std::runtime_error("Failed to read the executable path (GetModuleFileNameW)");
    }
    // A result equal to the buffer size means the path did not fit and was cut off.
    if (length >= MAX_LONG_PATH_LENGTH) {
        throw std::runtime_error("The executable path is too long (GetModuleFileNameW)");
    }

    // Cut the string down to the characters that were written. The path is built from the
    // wide string directly, without converting it to narrow characters.
    buffer.resize(length);
    return std::filesystem::path(buffer);
}

#endif

} // namespace

std::filesystem::path executableDir() {
    return executableFile().parent_path();
}

std::filesystem::path assetPath(const std::filesystem::path& relativePath) {
    // operator/ joins path parts with the separator of the current system.
    return executableDir() / ASSETS_DIRECTORY / relativePath;
}

} // namespace core
