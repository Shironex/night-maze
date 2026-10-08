// Search: the test behind the search box of the debug window.
#pragma once

#include <string_view>

namespace debug {

// This file knows nothing about ImGui: it is plain text handling, so the unit tests can
// check it (tests/SearchTests.cpp).

/// True when text contains every word of query. The words of the query are separated
/// by spaces, their order does not matter, and upper and lower case letters count as
/// the same: "BIAS moon" finds "Moon shadows, Constant bias". A word may be a part of
/// a longer word: "shad" finds "Shadows".
///
/// A query without any word (empty, or only spaces) matches every text.
bool matchesSearch(std::string_view text, std::string_view query);

/// True when the query has at least one word, so when a search is going on.
bool hasSearchWords(std::string_view query);

} // namespace debug
