// Tests of the search of the debug window, which is plain text handling without ImGui.
// See docs/modules/debug-ui.md
#include "debug/Search.hpp"

#include <doctest/doctest.h>

// What a row of the debug window hands to the search is one line of text: the place of
// its card, the title of the card, its own label and its help text. These tests use
// lines of that shape.

TEST_CASE("an empty search matches every text") {
    CHECK(debug::matchesSearch("Light / Shadows Moon shadows Constant bias", ""));
    CHECK(debug::matchesSearch("", ""));
    // Only spaces: no word to look for.
    CHECK(debug::matchesSearch("Fog Density", "   "));
}

TEST_CASE("a word is found anywhere in the text, also inside a longer word") {
    CHECK(debug::matchesSearch("Light / Shadows Moon shadows Constant bias", "bias"));
    CHECK(debug::matchesSearch("Light / Shadows Moon shadows Constant bias", "shad"));
    CHECK_FALSE(debug::matchesSearch("Light / Shadows Moon shadows Constant bias", "fog"));
}

TEST_CASE("upper and lower case letters count as the same") {
    CHECK(debug::matchesSearch("Post process Fog Density", "FOG"));
    CHECK(debug::matchesSearch("post process fog density", "Density"));
}

TEST_CASE("every word of the search has to be found, in any order") {
    const char* row = "Light / Shadows Flashlight shadows Slope bias";
    CHECK(debug::matchesSearch(row, "bias flashlight"));
    CHECK(debug::matchesSearch(row, "flashlight bias"));
    // One of the two words is missing.
    CHECK_FALSE(debug::matchesSearch(row, "bias moon"));
}

TEST_CASE("spaces around and between the words do not count") {
    CHECK(debug::matchesSearch("World / Maze Maze Seed", "  seed   maze "));
    CHECK_FALSE(debug::matchesSearch("World / Maze Maze Seed", "  seed   grass "));
}

TEST_CASE("a search word longer than the text is not found") {
    CHECK_FALSE(debug::matchesSearch("Fog", "Foggy"));
    CHECK_FALSE(debug::matchesSearch("", "fog"));
}

TEST_CASE("a search is going on only when the query has a word") {
    CHECK_FALSE(debug::hasSearchWords(""));
    CHECK_FALSE(debug::hasSearchWords("   "));
    CHECK(debug::hasSearchWords("bias"));
    CHECK(debug::hasSearchWords("  bias "));
}
