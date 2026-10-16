// GrassRenderer: draws the grass, in chunks, with instancing or with the geometry shader.
#include "game/GrassRenderer.hpp"

#include "core/GlCheck.hpp"
#include "game/MazeRenderer.hpp"
#include "game/ShaderUniforms.hpp"
#include "gfx/RenderStats.hpp"
#include "gfx/Shader.hpp"
#include "scene/Collider.hpp"
#include "scene/Frustum.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace game {

namespace {

// The grass has no highlight. common/lighting.glsl still computes one, so its two
// numbers are set to values that are safe to compute with: no strength, and an exponent
// of 1 (the power of 0 with an exponent of 0 is not defined in GLSL).
constexpr float NO_SPECULAR_STRENGTH = 0.0F;
constexpr float PLAIN_SHININESS = 1.0F;

// One entry of the buffer of tufts: what both vertex shaders read of a tuft.
struct TuftVertex {
    glm::vec3 root{0.0F};
    float random = 0.0F;
};

// The attribute numbers, as in layout(location = ...) of grass.vert and
// grass_instanced.vert: the corner of the mesh of a tuft, and the two numbers of a tuft.
constexpr GLuint BLADE_CORNER_ATTRIBUTE = 0;
constexpr GLuint TUFT_ROOT_ATTRIBUTE = 1;
constexpr GLuint TUFT_RANDOM_ATTRIBUTE = 2;
constexpr GLint BLADE_CORNER_COMPONENTS = 2;
constexpr GLint TUFT_ROOT_COMPONENTS = 3;
constexpr GLint TUFT_RANDOM_COMPONENTS = 1;
constexpr auto TUFT_STRIDE = static_cast<GLsizei>(sizeof(TuftVertex));
constexpr auto BLADE_CORNER_STRIDE = static_cast<GLsizei>(sizeof(glm::vec2));

// The mesh of one tuft: GRASS_BLADES_PER_TUFT blades of five corners (the corners of
// common/grass_blade.glsl: root left, root right, middle left, middle right, tip).
constexpr int CORNERS_PER_BLADE = 5;
constexpr int TUFT_CORNERS = GRASS_BLADES_PER_TUFT * CORNERS_PER_BLADE;

// The three triangles of one blade, as corners of that blade. They are the triangles of
// the strip the geometry shader writes (left, right, left, right, tip): every corner
// after the second one with the two before it.
constexpr int INDICES_PER_BLADE = 9;
constexpr std::array<std::uint32_t, INDICES_PER_BLADE> BLADE_TRIANGLES = {0, 1, 2, 2, 1,
                                                                          3, 2, 3, 4};

// The corners of the mesh: for every corner the number of its blade and its number in
// the blade, the two numbers grass_instanced.vert reads (aBladeCorner).
std::array<glm::vec2, TUFT_CORNERS> tuftCorners() {
    std::array<glm::vec2, TUFT_CORNERS> corners{};
    for (int blade = 0; blade < GRASS_BLADES_PER_TUFT; ++blade) {
        for (int corner = 0; corner < CORNERS_PER_BLADE; ++corner) {
            corners[static_cast<std::size_t>(blade * CORNERS_PER_BLADE + corner)] = {
                static_cast<float>(blade), static_cast<float>(corner)};
        }
    }
    return corners;
}

// The indices of the mesh: the triangles of the blades, blade after blade. That order
// is what the distance LOD relies on: the first 9 indices are the first blade, the
// first 18 the first two. Drawing fewer indices draws fewer blades of every tuft.
std::array<std::uint32_t, GRASS_BLADES_PER_TUFT * INDICES_PER_BLADE> tuftIndices() {
    std::array<std::uint32_t, GRASS_BLADES_PER_TUFT * INDICES_PER_BLADE> indices{};
    for (int blade = 0; blade < GRASS_BLADES_PER_TUFT; ++blade) {
        const auto first = static_cast<std::uint32_t>(blade * CORNERS_PER_BLADE);
        for (int i = 0; i < INDICES_PER_BLADE; ++i) {
            indices[static_cast<std::size_t>(blade * INDICES_PER_BLADE + i)] =
                first + BLADE_TRIANGLES[static_cast<std::size_t>(i)];
        }
    }
    return indices;
}

const std::array<glm::vec2, TUFT_CORNERS> TUFT_CORNER_DATA = tuftCorners();
const std::array<std::uint32_t, GRASS_BLADES_PER_TUFT * INDICES_PER_BLADE> TUFT_INDEX_DATA =
    tuftIndices();

} // namespace

GrassRenderer::GrassRenderer()
    // m_bladeArray is constructed first (it is declared first) and binds itself, so the
    // index buffer below is recorded in it.
    : m_bladeCorners(GL_ARRAY_BUFFER, TUFT_CORNER_DATA.data(), sizeof(TUFT_CORNER_DATA)),
      m_bladeIndices(GL_ELEMENT_ARRAY_BUFFER, TUFT_INDEX_DATA.data(), sizeof(TUFT_INDEX_DATA)),
      // An empty buffer of tufts: upload fills it.
      m_tuftBuffer(GL_ARRAY_BUFFER, nullptr, 0) {
    // The corners are read per vertex, from the buffer of the mesh.
    m_bladeCorners.bind();
    m_bladeArray.setFloatAttribute(BLADE_CORNER_ATTRIBUTE, BLADE_CORNER_COMPONENTS,
                                   BLADE_CORNER_STRIDE, 0);
    // The two numbers of a tuft move on once per instance. Where they are read from is
    // set before every draw call (drawInstanced): each range of chunks starts elsewhere
    // in the buffer. The divisor stays with the vertex array.
    m_bladeArray.setAttributeDivisor(TUFT_ROOT_ATTRIBUTE, 1);
    m_bladeArray.setAttributeDivisor(TUFT_RANDOM_ATTRIBUTE, 1);
}

void GrassRenderer::upload(std::span<const GrassTuft> tufts) {
    // The order of the tufts on the card is the order of the chunks. placeGrass keeps
    // its own order: only this copy is sorted.
    const GrassChunks grouped = groupGrassIntoChunks(tufts, GRASS_CHUNK_SIZE);
    std::vector<TuftVertex> vertices;
    vertices.reserve(grouped.tufts.size());
    for (const GrassTuft& tuft : grouped.tufts) {
        vertices.push_back({.root = tuft.position, .random = tuft.random});
    }
    m_tuftBuffer.setData(vertices.data(), vertices.size() * sizeof(TuftVertex));
    m_chunks = grouped.chunks;
    m_tuftCount = grouped.tufts.size();

    // The points of the geometry shader read the buffer from its start, one tuft per
    // vertex. setData has left the buffer bound, which is what the vertex array records.
    m_pointArray.setFloatAttribute(TUFT_ROOT_ATTRIBUTE, TUFT_ROOT_COMPONENTS, TUFT_STRIDE,
                                   offsetof(TuftVertex, root));
    m_pointArray.setFloatAttribute(TUFT_RANDOM_ATTRIBUTE, TUFT_RANDOM_COMPONENTS, TUFT_STRIDE,
                                   offsetof(TuftVertex, random));
}

void GrassRenderer::draw(const gfx::Shader& shader, const GrassSettings& settings,
                         const GrassFrame& frame) const {
    // Nothing to draw, or nothing to draw with (the load error was logged once, when the
    // shader was created).
    if (m_tuftCount == 0 || !shader.isValid()) {
        return;
    }

    // Which chunks are drawn, and with how many blades. A chunk outside the frustum gets
    // 0. The instanced path draws a far chunk with fewer blades: the indices of the mesh
    // are ordered blade by blade, so fewer indices are fewer blades. The geometry shader
    // leaves out the blades of each tuft itself, so for it every visible chunk gets the
    // same number and the ranges join as far as their order allows.
    const bool instanced = settings.path == GrassPath::Instanced;
    m_bladesPerChunk.resize(m_chunks.size());
    for (std::size_t i = 0; i < m_chunks.size(); ++i) {
        const GrassChunk& chunk = m_chunks[i];
        if (frame.frustum != nullptr) {
            const bool seen =
                scene::canBeSeen(*frame.frustum, grassChunkBox(chunk.roots, settings.bladeHeight,
                                                               settings.windStrength));
            gfx::countCullTest(seen);
            if (!seen) {
                m_bladesPerChunk[i] = 0;
                continue;
            }
        }
        // No tuft of the chunk is nearer to the eye than the nearest point of the box
        // around their roots, so none of them needs more blades than a tuft there.
        const float nearest = glm::distance(frame.eye, scene::closestPoint(chunk.roots, frame.eye));
        m_bladesPerChunk[i] = instanced ? grassBladesAt(nearest, settings) : GRASS_BLADES_PER_TUFT;
    }
    const std::vector<GrassDrawRange> ranges = grassDrawRanges(m_chunks, m_bladesPerChunk);
    if (ranges.empty()) {
        return;
    }

    // The uniforms belong to the program in use, so use() comes before the setters.
    shader.use();
    shader.setMat4(VIEW_UNIFORM, frame.view);
    shader.setMat4(PROJECTION_UNIFORM, frame.projection);
    shader.setFloat(GRASS_TIME_UNIFORM, frame.timeSeconds);
    shader.setFloat(GRASS_BLADE_HEIGHT_UNIFORM, settings.bladeHeight);
    shader.setFloat(GRASS_WIND_STRENGTH_UNIFORM, settings.windStrength);
    shader.setInt(GRASS_LOD_ENABLED_UNIFORM, settings.distanceLod ? 1 : 0);
    shader.setVec3(GRASS_EYE_POSITION_UNIFORM, frame.eye);
    shader.setFloat(GRASS_LOD_NEAR_UNIFORM, settings.lodNear);
    shader.setFloat(GRASS_LOD_FAR_UNIFORM, grassLodFar(settings));
    shader.setFloat(GRASS_LOD_FADE_UNIFORM, GRASS_LOD_FADE);
    shader.setInt(GRASS_LIT_UNIFORM, frame.lit ? 1 : 0);
    // The enum values are the numbers grass.frag compares uViewMode with.
    shader.setInt(VIEW_MODE_UNIFORM, static_cast<int>(frame.viewMode));
    shader.setFloat(SPECULAR_STRENGTH_UNIFORM, NO_SPECULAR_STRENGTH);
    shader.setFloat(SHININESS_UNIFORM, PLAIN_SHININESS);

    // Back-face culling throws away triangles that are seen from behind. A blade of
    // grass is one flat strip without a back side of its own, and half of the blades
    // would vanish. The game does not switch culling on today. If it ever does, the
    // grass still has to be drawn without it, and the switch is put back afterwards.
    GLboolean cullingWasOn = GL_FALSE;
    GL_CHECK(cullingWasOn = glIsEnabled(GL_CULL_FACE));
    if (cullingWasOn == GL_TRUE) {
        GL_CHECK(glDisable(GL_CULL_FACE));
    }

    if (instanced) {
        drawInstanced(ranges);
    } else {
        drawPoints(ranges);
    }

    if (cullingWasOn == GL_TRUE) {
        GL_CHECK(glEnable(GL_CULL_FACE));
    }
}

void GrassRenderer::drawInstanced(std::span<const GrassDrawRange> ranges) const {
    m_bladeArray.bind();
    // The buffer of tufts has to be the one bound to GL_ARRAY_BUFFER while the two tuft
    // attributes are pointed into it below.
    m_tuftBuffer.bind();
    for (const GrassDrawRange& range : ranges) {
        // OpenGL 4.1 has no "base instance" (glDrawElementsInstancedBaseInstance came with
        // 4.2): every draw call starts with the first entry its attribute points at. So
        // the two attributes are pointed at the first tuft of the range, before each call.
        // A pointer is only a few numbers stored in the vertex array: changing it is
        // cheap, and no data moves.
        const std::size_t firstByte =
            static_cast<std::size_t>(range.firstTuft) * sizeof(TuftVertex);
        m_bladeArray.setFloatAttribute(TUFT_ROOT_ATTRIBUTE, TUFT_ROOT_COMPONENTS, TUFT_STRIDE,
                                       firstByte + offsetof(TuftVertex, root));
        m_bladeArray.setFloatAttribute(TUFT_RANDOM_ATTRIBUTE, TUFT_RANDOM_COMPONENTS, TUFT_STRIDE,
                                       firstByte + offsetof(TuftVertex, random));

        // range.blades blades of every tuft: the first that many blades of the mesh.
        // The last parameter is the offset of the first index in the index buffer: 0,
        // passed as a pointer like in gfx::Mesh::draw.
        const auto indexCount = static_cast<GLsizei>(range.blades * INDICES_PER_BLADE);
        GL_CHECK(glDrawElementsInstanced(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr,
                                         static_cast<GLsizei>(range.tuftCount)));
        // One draw call. Its triangles are those of every copy.
        gfx::countDraw(GL_TRIANGLES, static_cast<std::size_t>(indexCount) * range.tuftCount);
    }
}

void GrassRenderer::drawPoints(std::span<const GrassDrawRange> ranges) const {
    m_pointArray.bind();
    for (const GrassDrawRange& range : ranges) {
        // One point per tuft. The geometry shader gets one point at a time.
        GL_CHECK(glDrawArrays(GL_POINTS, static_cast<GLint>(range.firstTuft),
                              static_cast<GLsizei>(range.tuftCount)));
        gfx::countDraw(GL_POINTS, range.tuftCount);
    }
}

} // namespace game
