// Search: the test behind the search box of the debug window.
// See docs/modules/debug-ui.md
#include "debug/Search.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>

namespace debug {

namespace {

constexpr char WORD_SEPARATOR = ' ';

// The lower case form of a letter, every other character unchanged. std::tolower takes
// the character as an unsigned number: a negative char (a letter outside ASCII) would be
// undefined behaviour, hence the cast.
char lowerCase(char character) {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
}

// True when word stands somewhere in text, ignoring upper and lower case. word must not
// be empty.
bool containsWord(std::string_view text, std::string_view word) {
    // std::ranges::search looks for the second range inside the first one and compares
    // the characters with the function it is given. It returns the place it found, as
    // a range of its own, and an empty range when there is no such place.
    const auto sameLetter = [](char left, char right) {
        return lowerCase(left) == lowerCase(right);
    };
    return !std::ranges::search(text, word, sameLetter).empty();
}

} // namespace

bool matchesSearch(std::string_view text, std::string_view query) {
    // Walks over the query word by word. start is where the next word begins.
    std::size_t start = 0;
    while (start < query.size()) {
        // The end of the word: the next space, or the end of the query (npos).
        const std::size_t end = query.find(WORD_SEPARATOR, start);
        const std::string_view word = query.substr(start, end - start);
        // Two spaces in a row give an empty word, which is skipped.
        if (!word.empty() && !containsWord(text, word)) {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    return true;
}

bool hasSearchWords(std::string_view query) {
    // find_first_not_of returns npos when every character is a space.
    return query.find_first_not_of(WORD_SEPARATOR) != std::string_view::npos;
}

} // namespace debug
