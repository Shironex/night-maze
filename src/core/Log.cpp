// Minimal console logging: one line per message with a severity prefix.
#include "core/Log.hpp"

#include <iostream>

namespace core {

// Every message ends with std::flush, which writes the buffered text out right away, so
// messages appear immediately and are not lost if the program crashes or its output is
// redirected to a file.
void logInfo(std::string_view message) {
    std::cout << "[info] " << message << '\n' << std::flush;
}

void logWarn(std::string_view message) {
    std::cerr << "[warn] " << message << '\n' << std::flush;
}

void logError(std::string_view message) {
    std::cerr << "[error] " << message << '\n' << std::flush;
}

} // namespace core
