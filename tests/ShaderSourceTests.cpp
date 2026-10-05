// Tests of gfx::expandIncludes and gfx::nameSourceFiles: #include in shader files.
// See docs/modules/gfx/shader-includes.md
#include "gfx/ShaderSource.hpp"

#include <doctest/doctest.h>

#include <map>
#include <string>
#include <vector>

namespace {

// The "files" of a test: name and contents. The reader given to expandIncludes looks
// the name up here instead of opening a file.
using Files = std::map<std::string, std::string>;

gfx::IncludeReader readerOf(const Files& files) {
    return [&files](const std::string& name, std::string& text) {
        const auto found = files.find(name);
        if (found == files.end()) {
            return false;
        }
        text = found->second;
        return true;
    };
}

// The result of one call, so that a test can look at all three outputs.
struct Expanded {
    bool ok = false;
    gfx::ShaderSource source;
    std::string error;
};

Expanded expand(const std::string& rootText, const Files& files) {
    Expanded result;
    result.ok =
        gfx::expandIncludes("main.frag", rootText, readerOf(files), result.source, result.error);
    return result;
}

} // namespace

TEST_CASE("a shader without #include goes through unchanged") {
    const Expanded result = expand("#version 410 core\nvoid main() {\n}\n", {});

    REQUIRE(result.ok);
    CHECK(result.source.text == "#version 410 core\nvoid main() {\n}\n");
    REQUIRE(result.source.files.size() == 1);
    CHECK(result.source.files[0] == "main.frag");
    CHECK(result.error.empty());
}

TEST_CASE("a last line without a line end gets one") {
    const Expanded result = expand("#version 410 core\nvoid main() {}", {});

    REQUIRE(result.ok);
    CHECK(result.source.text == "#version 410 core\nvoid main() {}\n");
}

TEST_CASE("an #include line is replaced by the file between two #line directives") {
    const Files files = {{"common/light.glsl", "float a;\nfloat b;\n"}};
    const Expanded result = expand("#version 410 core\n"              // line 1
                                   "// comment\n"                     // line 2
                                   "#include \"common/light.glsl\"\n" // line 3
                                   "void main() {\n"                  // line 4
                                   "}\n",
                                   files);

    REQUIRE(result.ok);
    // The included file is source string number 1 and starts at its line 1. After it
    // the shader file (number 0) goes on with line 4, the line after the #include.
    CHECK(result.source.text == "#version 410 core\n"
                                "// comment\n"
                                "#line 1 1\n"
                                "float a;\n"
                                "float b;\n"
                                "#line 4 0\n"
                                "void main() {\n"
                                "}\n");
    REQUIRE(result.source.files.size() == 2);
    CHECK(result.source.files[0] == "main.frag");
    CHECK(result.source.files[1] == "common/light.glsl");
}

TEST_CASE("#version stays the first line of the result") {
    const Files files = {{"a.glsl", "float a;\n"}};
    const Expanded result = expand("#version 410 core\n#include \"a.glsl\"\n", files);

    REQUIRE(result.ok);
    CHECK(result.source.text.starts_with("#version 410 core\n"));
}

TEST_CASE("an included file may include another file") {
    const Files files = {
        {"outer.glsl", "float before;\n#include \"inner.glsl\"\nfloat after;\n"},
        {"inner.glsl", "float inner;\n"},
    };
    const Expanded result = expand("#version 410 core\n"
                                   "#include \"outer.glsl\"\n"
                                   "void main() {}\n",
                                   files);

    REQUIRE(result.ok);
    CHECK(result.source.text == "#version 410 core\n"
                                "#line 1 1\n" // outer.glsl starts
                                "float before;\n"
                                "#line 1 2\n" // inner.glsl starts
                                "float inner;\n"
                                "#line 3 1\n" // back in outer.glsl, at its line 3
                                "float after;\n"
                                "#line 3 0\n" // back in main.frag, at its line 3
                                "void main() {}\n");
    REQUIRE(result.source.files.size() == 3);
    CHECK(result.source.files[1] == "outer.glsl");
    CHECK(result.source.files[2] == "inner.glsl");
}

TEST_CASE("an included file without a line end at its end does not swallow #line") {
    const Files files = {{"a.glsl", "float a;"}};
    const Expanded result = expand("#version 410 core\n#include \"a.glsl\"\nfloat b;\n", files);

    REQUIRE(result.ok);
    CHECK(result.source.text == "#version 410 core\n"
                                "#line 1 1\n"
                                "float a;\n"
                                "#line 3 0\n"
                                "float b;\n");
}

TEST_CASE("the same file included twice keeps one number") {
    const Files files = {{"a.glsl", "// a\n"}};
    const Expanded result =
        expand("#version 410 core\n#include \"a.glsl\"\n#include \"a.glsl\"\n", files);

    REQUIRE(result.ok);
    CHECK(result.source.files.size() == 2);
    CHECK(result.source.text == "#version 410 core\n"
                                "#line 1 1\n"
                                "// a\n"
                                "#line 3 0\n"
                                "#line 1 1\n"
                                "// a\n"
                                "#line 4 0\n");
}

TEST_CASE("blanks around the # and Windows line ends are accepted") {
    const Files files = {{"a.glsl", "float a;\r\n"}};
    const Expanded result = expand("#version 410 core\r\n  #  include   \"a.glsl\"  \r\n", files);

    REQUIRE(result.ok);
    CHECK(result.source.text == "#version 410 core\n"
                                "#line 1 1\n"
                                "float a;\n"
                                "#line 3 0\n");
}

TEST_CASE("an #include in a line comment is left alone") {
    const Expanded result = expand("#version 410 core\n// #include \"missing.glsl\"\n", {});

    REQUIRE(result.ok);
    CHECK(result.source.text == "#version 410 core\n// #include \"missing.glsl\"\n");
}

TEST_CASE("a missing included file is an error with the file and the line") {
    const Expanded result = expand("#version 410 core\n\n#include \"missing.glsl\"\n", {});

    CHECK_FALSE(result.ok);
    CHECK(result.error == "main.frag:3: included file cannot be opened: \"missing.glsl\"");
}

TEST_CASE("a missing file inside an included file is reported against that file") {
    const Files files = {{"outer.glsl", "float a;\n#include \"missing.glsl\"\n"}};
    const Expanded result = expand("#version 410 core\n#include \"outer.glsl\"\n", files);

    CHECK_FALSE(result.ok);
    CHECK(result.error == "outer.glsl:2: included file cannot be opened: \"missing.glsl\"");
}

TEST_CASE("a file that includes itself is an error") {
    const Files files = {{"loop.glsl", "#include \"loop.glsl\"\n"}};
    const Expanded result = expand("#version 410 core\n#include \"loop.glsl\"\n", files);

    CHECK_FALSE(result.ok);
    CHECK(result.error == "loop.glsl:1: #include cycle: \"loop.glsl\" is already being included, "
                          "a file cannot include itself");
}

TEST_CASE("two files that include each other are an error") {
    const Files files = {
        {"a.glsl", "float a;\n#include \"b.glsl\"\n"},
        {"b.glsl", "float b;\nfloat c;\n#include \"a.glsl\"\n"},
    };
    const Expanded result = expand("#version 410 core\n#include \"a.glsl\"\n", files);

    CHECK_FALSE(result.ok);
    CHECK(result.error.starts_with("b.glsl:3: #include cycle: \"a.glsl\""));
}

TEST_CASE("an included file that includes the shader file is a cycle too") {
    const Files files = {{"a.glsl", "#include \"main.frag\"\n"}};
    const Expanded result = expand("#version 410 core\n#include \"a.glsl\"\n", files);

    CHECK_FALSE(result.ok);
    CHECK(result.error.starts_with("a.glsl:1: #include cycle: \"main.frag\""));
}

TEST_CASE("a malformed #include is an error") {
    CHECK_FALSE(expand("#version 410 core\n#include common/light.glsl\n", {}).ok);
    CHECK_FALSE(expand("#version 410 core\n#include \"unclosed\n", {}).ok);
    CHECK_FALSE(expand("#version 410 core\n#include \"\"\n", {}).ok);
    CHECK_FALSE(expand("#version 410 core\n#include\n", {}).ok);

    const Expanded result = expand("#version 410 core\n#include <a.glsl>\n", {});
    CHECK(result.error == "main.frag:2: malformed #include, expected #include \"file name\"");
}

TEST_CASE("#include before #version is an error") {
    const Files files = {{"a.glsl", "float a;\n"}};
    const Expanded result = expand("#include \"a.glsl\"\n#version 410 core\n", files);

    CHECK_FALSE(result.ok);
    CHECK(result.error.starts_with("main.frag:1: #include must come after the #version line"));
}

TEST_CASE("#version in an included file is an error") {
    const Files files = {{"a.glsl", "// a\n#version 410 core\n"}};
    const Expanded result = expand("#version 410 core\n#include \"a.glsl\"\n", files);

    CHECK_FALSE(result.ok);
    CHECK(result.error == "a.glsl:2: an included file must not have a #version line");
}

TEST_CASE("nameSourceFiles puts the file name into an NVIDIA error line") {
    const std::vector<std::string> files = {"lit.frag", "common/lighting.glsl"};

    CHECK(gfx::nameSourceFiles("1(15) : error C1503: undefined variable \"x\"\n", files) ==
          "common/lighting.glsl(15) : error C1503: undefined variable \"x\"\n"
          "Source files: 0 = lit.frag, 1 = common/lighting.glsl\n");
    CHECK(
        gfx::nameSourceFiles("0(7) : warning C7050: \"y\" might be used before being set\n", files)
            .starts_with("lit.frag(7) : warning C7050"));
}

TEST_CASE("nameSourceFiles puts the file name into an Apple error line") {
    const std::vector<std::string> files = {"lit.frag", "common/lighting.glsl"};

    CHECK(gfx::nameSourceFiles("ERROR: 1:15: Use of undeclared identifier 'x'\n", files) ==
          "ERROR: common/lighting.glsl:15: Use of undeclared identifier 'x'\n"
          "Source files: 0 = lit.frag, 1 = common/lighting.glsl\n");
    CHECK(gfx::nameSourceFiles("WARNING: 0:3: unused\n", files)
              .starts_with("WARNING: lit.frag:3: unused\n"));
    // Mesa and Intel write the same pair without a prefix.
    CHECK(gfx::nameSourceFiles("1:15(3): error: syntax error\n", files)
              .starts_with("common/lighting.glsl:15(3): error: syntax error\n"));
}

TEST_CASE("nameSourceFiles handles every line of a log") {
    const std::vector<std::string> files = {"lit.frag", "common/lighting.glsl"};

    CHECK(gfx::nameSourceFiles("1(15) : error C0000: first\n0(22) : error C0000: second\n",
                               files) == "common/lighting.glsl(15) : error C0000: first\n"
                                         "lit.frag(22) : error C0000: second\n"
                                         "Source files: 0 = lit.frag, 1 = common/lighting.glsl\n");
}

TEST_CASE("nameSourceFiles leaves lines it does not recognise alone") {
    const std::vector<std::string> files = {"lit.frag", "common/lighting.glsl"};
    const std::string legend = "Source files: 0 = lit.frag, 1 = common/lighting.glsl\n";

    // An unknown format, a number that is not in the list, a number without a line
    // number after it, plain text and an empty line.
    CHECK(gfx::nameSourceFiles("error at <1, 15>: something\n", files) ==
          "error at <1, 15>: something\n" + legend);
    CHECK(gfx::nameSourceFiles("7(15) : error C0000: x\n", files) ==
          "7(15) : error C0000: x\n" + legend);
    CHECK(gfx::nameSourceFiles("1 error generated\n", files) == "1 error generated\n" + legend);
    CHECK(gfx::nameSourceFiles("1(a) : odd\n", files) == "1(a) : odd\n" + legend);
    CHECK(gfx::nameSourceFiles("link failed\n\n", files) == "link failed\n\n" + legend);
    CHECK(gfx::nameSourceFiles("1", files) == "1\n" + legend);
    CHECK(gfx::nameSourceFiles("", files) == legend);
}

TEST_CASE("nameSourceFiles adds no list of files for a shader without includes") {
    const std::vector<std::string> files = {"color.frag"};

    CHECK(gfx::nameSourceFiles("0(4) : error C0000: x\n", files) ==
          "color.frag(4) : error C0000: x\n");
}
