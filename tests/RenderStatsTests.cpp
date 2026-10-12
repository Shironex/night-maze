// Tests of gfx/RenderStats: the counters of a frame. They are plain numbers and need no
// OpenGL context: nothing here calls OpenGL.
#include "gfx/RenderStats.hpp"

#include <doctest/doctest.h>

TEST_CASE("only a draw call of triangles has triangles, three vertices each") {
    CHECK(gfx::trianglesOf(GL_TRIANGLES, 36) == 12);
    CHECK(gfx::trianglesOf(GL_TRIANGLES, 3) == 1);
    CHECK(gfx::trianglesOf(GL_TRIANGLES, 0) == 0);
    // Lines (the collision boxes) and points (the grass) are draw calls without
    // triangles.
    CHECK(gfx::trianglesOf(GL_LINES, 24) == 0);
    CHECK(gfx::trianglesOf(GL_POINTS, 500) == 0);
}

TEST_CASE("the counters add up what was counted since the reset") {
    gfx::resetRenderCounters();
    gfx::countDraw(GL_TRIANGLES, 36);
    gfx::countDraw(GL_TRIANGLES, 6);
    gfx::countDraw(GL_POINTS, 500);
    gfx::countUniformLookup();
    gfx::countUniformLookup();
    gfx::countTextureBind();

    const gfx::RenderCounters& counters = gfx::renderCounters();
    CHECK(counters.drawCalls == 3);
    CHECK(counters.triangles == 14);
    CHECK(counters.uniformLookups == 2);
    CHECK(counters.textureBinds == 1);
}

TEST_CASE("a reset sets every counter to zero") {
    gfx::countDraw(GL_TRIANGLES, 3);
    gfx::countUniformLookup();
    gfx::countTextureBind();
    gfx::resetRenderCounters();

    const gfx::RenderCounters& counters = gfx::renderCounters();
    CHECK(counters.drawCalls == 0);
    CHECK(counters.triangles == 0);
    CHECK(counters.uniformLookups == 0);
    CHECK(counters.textureBinds == 0);
}

TEST_CASE("countersSince is what was counted between two readings") {
    gfx::resetRenderCounters();
    gfx::countDraw(GL_TRIANGLES, 30);
    gfx::countUniformLookup();
    // A copy: the reading at the start of a pass.
    const gfx::RenderCounters start = gfx::renderCounters();

    gfx::countDraw(GL_TRIANGLES, 12);
    gfx::countDraw(GL_LINES, 24);
    gfx::countUniformLookup();
    gfx::countUniformLookup();
    gfx::countTextureBind();

    const gfx::RenderCounters pass = gfx::countersSince(gfx::renderCounters(), start);
    CHECK(pass.drawCalls == 2);
    CHECK(pass.triangles == 4);
    CHECK(pass.uniformLookups == 2);
    CHECK(pass.textureBinds == 1);
}
