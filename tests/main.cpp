// Entry point of the test program night_maze_tests.
// See docs/libraries/doctest.md

// doctest is a single header. In exactly one .cpp file of the program this macro makes
// the header also emit its implementation and a main() function that runs every
// TEST_CASE of every file. The other test files include the header without the macro.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
