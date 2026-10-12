// RenderStats: counters of what the game asks of the graphics card in one frame.
#include "gfx/RenderStats.hpp"

namespace gfx {

namespace {

// A triangle is three vertices.
constexpr std::size_t VERTICES_PER_TRIANGLE = 3;

// The one set of counters of the program (see RenderStats.hpp for why it is one).
RenderCounters counters;

} // namespace

std::uint32_t trianglesOf(GLenum primitive, std::size_t count) {
    if (primitive != GL_TRIANGLES) {
        return 0;
    }
    return static_cast<std::uint32_t>(count / VERTICES_PER_TRIANGLE);
}

RenderCounters countersSince(const RenderCounters& now, const RenderCounters& start) {
    return {.drawCalls = now.drawCalls - start.drawCalls,
            .triangles = now.triangles - start.triangles,
            .uniformLookups = now.uniformLookups - start.uniformLookups,
            .textureBinds = now.textureBinds - start.textureBinds};
}

const RenderCounters& renderCounters() {
    return counters;
}

void resetRenderCounters() {
    counters = RenderCounters{};
}

void countDraw(GLenum primitive, std::size_t count) {
    counters.drawCalls += 1;
    counters.triangles += trianglesOf(primitive, count);
}

void countUniformLookup() {
    counters.uniformLookups += 1;
}

void countTextureBind() {
    counters.textureBinds += 1;
}

} // namespace gfx
