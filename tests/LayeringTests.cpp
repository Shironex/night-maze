// Tests of the layering rules of docs/architecture.md, read from the sources: which folder of
// src/ may include which, and that the rule files of game_logic stay free of OpenGL, the
// window and the debug UI. A failure names every include that breaks a rule. An include that
// is allowed against a rule is listed below by name, with the reason, so it cannot spread.
#include "core/Files.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {

// The root of the repository, found as the folder that holds assets/.
std::filesystem::path repositoryRoot() {
    return std::filesystem::path(NIGHT_MAZE_ASSETS_DIR).parent_path();
}

std::filesystem::path sourceRoot() {
    return repositoryRoot() / "src";
}

// The folders of the engine and of the three libraries beside it: they never include the
// game or the debug UI.
constexpr std::array<std::string_view, 7> ENGINE_FOLDERS = {"core",  "gfx", "scene", "assets",
                                                            "audio", "ui",  "video"};

// The folders a folder of the engine may include besides itself (architecture.md, section 1).
struct AllowedFolders {
    std::string_view folder;
    std::array<std::string_view, 4> may;
};
constexpr std::array<AllowedFolders, 7> ENGINE_ALLOWED = {{
    {.folder = "core", .may = {}},
    {.folder = "scene", .may = {}},
    {.folder = "gfx", .may = {"core"}},
    {.folder = "assets", .may = {"core", "gfx", "scene"}},
    {.folder = "ui", .may = {"core", "gfx", "scene", "assets"}},
    {.folder = "video", .may = {"core", "gfx", "scene", "assets"}},
    {.folder = "audio", .may = {"core", "gfx", "scene", "assets"}},
}};

// The headers of gfx/ that hold plain data and math and no OpenGL call, which the rules may
// use: the layout of a vertex, the sRGB conversion and the fit of a picture into a frame.
constexpr std::array<std::string_view, 3> GFX_PLAIN_DATA = {"gfx/Vertex.hpp", "gfx/ColorSpace.hpp",
                                                            "gfx/CoverFit.hpp"};

// The headers of core/ that open the window or talk to OpenGL.
constexpr std::array<std::string_view, 3> CORE_WINDOW = {"core/Application.hpp", "core/Window.hpp",
                                                         "core/GlCheck.hpp"};

// Known exception: the key bindings include GLFW only for the numbers of the keys
// (GLFW_KEY_W and the others) as default bindings. Nothing in the file opens a window or
// calls GLFW, and the test program links it without one.
constexpr std::array<std::string_view, 1> LOGIC_GLFW_ALLOWED = {"game/KeyBindings.cpp"};

// One #include line of a source file.
struct Include {
    // The including file, relative to src/ ("game/Round.cpp").
    std::string file;
    // What it includes, as written between the quotes or the angle brackets.
    std::string path;
    bool quoted = false;
};

// The first folder of a path relative to src/ ("game" for "game/Round.hpp"), empty for a file
// straight in src/ or a header of a library without a folder.
std::string topFolder(std::string_view path) {
    const std::size_t slash = path.find('/');
    return slash == std::string_view::npos ? std::string() : std::string(path.substr(0, slash));
}

template <typename Container>
bool contains(const Container& container, std::string_view value) {
    return std::ranges::find(container, value) != container.end();
}

// The lines of a text file, without the line ends and the leading spaces.
std::vector<std::string_view> trimmedLines(const std::string& text) {
    std::vector<std::string_view> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        end = end == std::string::npos ? text.size() : end;
        std::string_view line(text.data() + start, end - start);
        start = end + 1;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.remove_suffix(1);
        }
        line.remove_prefix(std::min(line.find_first_not_of(" \t"), line.size()));
        lines.push_back(line);
    }
    return lines;
}

// Every #include of every .cpp, .hpp and .mm file under src/.
std::vector<Include> readIncludes() {
    std::vector<Include> includes;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(sourceRoot())) {
        const std::filesystem::path& path = entry.path();
        const std::filesystem::path extension = path.extension();
        if (!entry.is_regular_file() ||
            (extension != ".cpp" && extension != ".hpp" && extension != ".mm")) {
            continue;
        }
        std::string text;
        REQUIRE(core::readTextFile(path, text));
        const std::string file = path.lexically_relative(sourceRoot()).generic_string();
        for (const std::string_view line : trimmedLines(text)) {
            if (!line.starts_with("#include")) {
                continue;
            }
            const std::size_t open = line.find_first_of("\"<");
            if (open == std::string_view::npos) {
                continue;
            }
            const char close = line[open] == '"' ? '"' : '>';
            const std::size_t closing = line.find(close, open + 1);
            if (closing == std::string_view::npos) {
                continue;
            }
            includes.push_back({.file = file,
                                .path = std::string(line.substr(open + 1, closing - open - 1)),
                                .quoted = close == '"'});
        }
    }
    return includes;
}

// True for the first line of a block of the CMake lists that names the sources of target:
// "add_library(target STATIC", "add_executable(target" or "target_sources(target PRIVATE".
bool startsSourceBlock(std::string_view line, std::string_view target) {
    const std::string name(target);
    return line == "add_library(" + name + " STATIC" || line == "add_executable(" + name ||
           line == "target_sources(" + name + " PRIVATE";
}

// The files under src/ that target lists, relative to src/, read from every CMakeLists.txt of
// the repository root, src/ and tests/. A file name in a list is relative to the folder of
// that list.
std::set<std::string> filesOfTarget(std::string_view target) {
    std::vector<std::filesystem::path> lists = {repositoryRoot() / "CMakeLists.txt"};
    for (const char* folder : {"src", "tests"}) {
        for (const std::filesystem::directory_entry& entry :
             std::filesystem::recursive_directory_iterator(repositoryRoot() / folder)) {
            if (entry.path().filename() == "CMakeLists.txt") {
                lists.push_back(entry.path());
            }
        }
    }
    std::set<std::string> files;
    for (const std::filesystem::path& list : lists) {
        std::string text;
        REQUIRE(core::readTextFile(list, text));
        bool inside = false;
        for (const std::string_view line : trimmedLines(text)) {
            if (!inside) {
                inside = startsSourceBlock(line, target);
            } else if (line == ")") {
                inside = false;
            } else if (!line.empty() && line.front() != '#') {
                const std::filesystem::path file =
                    (list.parent_path() / std::string(line)).lexically_normal();
                const std::string underSource =
                    file.lexically_relative(sourceRoot()).generic_string();
                if (!underSource.starts_with("..")) {
                    files.insert(underSource);
                }
            }
        }
    }
    return files;
}

// Our own folders under src/.
std::set<std::string> ourFolders() {
    std::set<std::string> folders;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(sourceRoot())) {
        if (entry.is_directory()) {
            folders.insert(entry.path().filename().generic_string());
        }
    }
    return folders;
}

std::string describe(const Include& include) {
    return include.file + " includes " + include.path;
}

// "file includes path", one per broken rule, for the failure message.
std::string listed(const std::vector<std::string>& lines) {
    std::string text;
    for (const std::string& line : lines) {
        text += "\n  ";
        text += line;
    }
    return text;
}

// True for a header that needs a window, an OpenGL context or the debug UI.
bool needsWindow(const Include& include, bool glfwAllowed) {
    const std::string& path = include.path;
    const std::string folder = topFolder(path);
    if (path.starts_with("glad/") || path.starts_with("imgui") || path.starts_with("RmlUi/") ||
        path.starts_with("miniaudio")) {
        return true;
    }
    if (path.starts_with("GLFW/")) {
        return !glfwAllowed;
    }
    // A file's own folder is never in the way: the rule is about what it reaches out to.
    if (!include.quoted || folder == topFolder(include.file)) {
        return false;
    }
    if (folder == "gfx") {
        return !contains(GFX_PLAIN_DATA, path);
    }
    return contains(CORE_WINDOW, path) || path == "assets/AssetCache.hpp" || folder == "debug" ||
           folder == "ui" || folder == "video" || folder == "audio";
}

} // namespace

TEST_CASE("the layering check reads the sources and the lists") {
    const std::vector<Include> includes = readIncludes();
    CHECK(includes.size() > 1000);
    const std::set<std::string> folders = ourFolders();
    for (const std::string_view folder : {"game", "debug"}) {
        CHECK(folders.contains(std::string(folder)));
    }
    for (const std::string_view folder : ENGINE_FOLDERS) {
        CHECK(folders.contains(std::string(folder)));
    }
    const std::set<std::string> logic = filesOfTarget("game_logic");
    CHECK(logic.contains("game/Round.cpp"));
    CHECK_FALSE(logic.contains("game/MazeRenderer.cpp"));
    CHECK(filesOfTarget("night_maze").contains("main.cpp"));
    CHECK(filesOfTarget("night_maze_tests").contains("debug/Search.cpp"));
}

TEST_CASE("the engine's folders never include the game or the debug ui") {
    std::vector<std::string> broken;
    for (const Include& include : readIncludes()) {
        const std::string folder = topFolder(include.path);
        if (contains(ENGINE_FOLDERS, topFolder(include.file)) && include.quoted &&
            (folder == "game" || folder == "debug")) {
            broken.push_back(describe(include));
        }
    }
    CHECK_MESSAGE(broken.empty(), listed(broken));
}

TEST_CASE("each folder of the engine includes only the folders below it") {
    const std::set<std::string> folders = ourFolders();
    std::vector<std::string> broken;
    for (const Include& include : readIncludes()) {
        const std::string from = topFolder(include.file);
        const std::string to = topFolder(include.path);
        if (!include.quoted || !folders.contains(to) || from == to) {
            continue;
        }
        const auto rule = std::ranges::find(ENGINE_ALLOWED, from, &AllowedFolders::folder);
        if (rule != ENGINE_ALLOWED.end() && !contains(rule->may, to)) {
            broken.push_back(describe(include));
        }
    }
    CHECK_MESSAGE(broken.empty(), listed(broken));
}

TEST_CASE("the game never includes the debug ui") {
    std::vector<std::string> broken;
    for (const Include& include : readIncludes()) {
        if (topFolder(include.file) == "game" && include.quoted &&
            topFolder(include.path) == "debug") {
            broken.push_back(describe(include));
        }
    }
    CHECK_MESSAGE(broken.empty(), listed(broken));
}

TEST_CASE("the rules of game_logic include no opengl, window or debug ui header") {
    const std::set<std::string> logic = filesOfTarget("game_logic");
    REQUIRE(!logic.empty());
    std::vector<std::string> broken;
    for (const Include& include : readIncludes()) {
        if (logic.contains(include.file) &&
            needsWindow(include, contains(LOGIC_GLFW_ALLOWED, include.file))) {
            broken.push_back(describe(include));
        }
    }
    CHECK_MESSAGE(broken.empty(), listed(broken));
}

TEST_CASE("the files of src that the test program compiles need no window") {
    const std::set<std::string> tested = filesOfTarget("night_maze_tests");
    REQUIRE(!tested.empty());
    std::vector<std::string> broken;
    for (const Include& include : readIncludes()) {
        if (tested.contains(include.file) && needsWindow(include, false)) {
            broken.push_back(describe(include));
        }
    }
    CHECK_MESSAGE(broken.empty(), listed(broken));
}
