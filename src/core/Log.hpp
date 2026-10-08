// Minimal console logging: one line per message with a severity prefix.
#pragma once

#include <string_view>

namespace core {

/// Prints "[info] message" to standard output.
void logInfo(std::string_view message);

/// Prints "[warn] message" to standard error.
void logWarn(std::string_view message);

/// Prints "[error] message" to standard error.
void logError(std::string_view message);

} // namespace core
