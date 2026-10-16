// GrassRenderer: draws the grass, in chunks, with instancing or with the geometry shader.
#pragma once

#include "game/Grass.hpp"
#include "gfx/Buffer.hpp"
#include "gfx/VertexArray.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct Frustum;
} // namespace scene

namespace game {

enum class ViewMode;

/// What a frame tells the grass, apart from its settings.
struct GrassFrame {
    /// The matrices of the frame, and the eye they look from (for the distance LOD).
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    glm::vec3 eye{0.0F};
    /// The frustum the chunks are tested against, or nullptr: then every chunk is drawn
    /// (the chunk culling or the frustum culling of the game is switched off).
    const scene::Frustum* frustum = nullptr;
    /// The clock of the wind: any time that keeps growing.
    float timeSeconds = 0.0F;
    /// False in the lighting mode Unlit, where the grass is shown at full brightness like
    /// everything else.
    bool lit = true;
    /// The debug views, as for the walls.
    ViewMode viewMode{};
};

/// The OpenGL side of the grass. Where the tufts stand is plain data (game::placeGrass).
/// This class copies them into one buffer on the graphics card, one entry per tuft
/// (its root and its random number), ordered by chunk (game::groupGrassIntoChunks).
/// The blades themselves are in no buffer: the graphics card makes them while the frame
/// is drawn, in one of two ways (GrassPath):
///
///   - Instanced: one small mesh with the corners of the three blades of a tuft is drawn
///     once per tuft (glDrawElementsInstanced). The buffer of tufts is read as an
///     attribute with a divisor of 1, so every copy gets the root of its own tuft
///     (grass_instanced.vert).
///   - GeometryShader: the buffer of tufts is drawn as GL_POINTS, and the geometry shader
///     grass.geom turns every point into the blades of a tuft.
///
/// Both read the same buffer and draw the same chunks, so the switch between them costs
/// nothing.
///
/// It owns OpenGL objects, so it must be destroyed before the window.
class GrassRenderer {
public:
    /// Makes the mesh of one tuft. There are no tufts yet: nothing is drawn until upload
    /// was called.
    GrassRenderer();

    /// Replaces the tufts on the graphics card by the given ones, grouped into chunks of
    /// GRASS_CHUNK_SIZE. Call it whenever the tufts were placed again: for a new maze,
    /// a new terrain or a new density.
    void upload(std::span<const GrassTuft> tufts);

    /// Number of tufts on the graphics card.
    std::size_t tuftCount() const { return m_tuftCount; }

    /// Number of chunks the tufts are grouped into.
    std::size_t chunkCount() const { return m_chunks.size(); }

    /// Draws the grass with shader, which must be the program of settings.path: the one
    /// of grass_instanced.vert and grass.frag for GrassPath::Instanced, the one of
    /// grass.vert, grass.geom and grass.frag for GrassPath::GeometryShader. It selects
    /// the program and sets all of its uniforms itself. The lights come from the uniform
    /// buffer of game::LightRig, which must be filled for this frame and connected to the
    /// program.
    ///
    /// With frame.frustum, a chunk whose box (grassChunkBox) lies outside of it is left
    /// out, and every chunk that is tested is counted (gfx::countCullTest). The chunks
    /// that are drawn become as few draw calls as their order allows
    /// (grassDrawRanges). With the distance LOD on (settings.distanceLod) the instanced
    /// path also draws a far chunk with fewer blades per tuft (grassBladesAt). The shader
    /// shrinks the blades away smoothly in both paths.
    ///
    /// A blade is a flat strip that is seen from both sides. If back-face culling is
    /// switched on, it is switched off for this draw and on again afterwards.
    void draw(const gfx::Shader& shader, const GrassSettings& settings,
              const GrassFrame& frame) const;

private:
    // The instanced draw calls and the geometry shader draw calls of a list of ranges.
    void drawInstanced(std::span<const GrassDrawRange> ranges) const;
    void drawPoints(std::span<const GrassDrawRange> ranges) const;

    // Order matters: members are constructed top to bottom. The vertex array of the mesh
    // comes before the two buffers of the mesh, so the index buffer is recorded in it.
    // mutable: draw points its two tuft attributes at another range before every draw
    // call. That is state of the drawing, not of the grass: the next draw sets it again.
    mutable gfx::VertexArray m_bladeArray;
    gfx::Buffer m_bladeCorners;
    gfx::Buffer m_bladeIndices;
    // The vertex array that reads the buffer of tufts as points, for the geometry shader.
    gfx::VertexArray m_pointArray;
    // One entry per tuft, ordered by chunk: read per instance by m_bladeArray and per
    // vertex by m_pointArray.
    gfx::Buffer m_tuftBuffer;

    std::vector<GrassChunk> m_chunks;
    std::size_t m_tuftCount = 0;

    // How many blades every chunk gets in this frame: kept between frames so drawing
    // allocates nothing once it has the size of the chunk list.
    mutable std::vector<int> m_bladesPerChunk;
};

} // namespace game
