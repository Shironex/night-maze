// GrassRenderer: draws the grass, one point per tuft that the geometry shader turns into blades.
// See docs/modules/renderer/grass-geometry.md
#pragma once

#include "gfx/Mesh.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <span>

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

enum class ViewMode;
struct GrassSettings;
struct GrassTuft;

/// The OpenGL side of the grass. Where the tufts stand is plain data (game::placeGrass).
/// This class owns the vertex buffer they are copied into: one vertex per tuft, drawn as
/// GL_POINTS. The blades are not in any buffer: the geometry shader grass.geom makes
/// them from the points while the frame is drawn.
///
/// It owns an OpenGL object, so it must be destroyed before the window.
class GrassRenderer {
public:
    /// Starts without tufts: nothing is drawn until upload was called.
    GrassRenderer();

    /// Replaces the points on the graphics card by the given tufts. Call it whenever
    /// the tufts were placed again: for a new maze, a new terrain or a new density.
    void upload(std::span<const GrassTuft> tufts);

    /// Number of tufts on the graphics card.
    std::size_t tuftCount() const { return m_tuftCount; }

    /// Draws the grass with the grass program (grass.vert, grass.geom and grass.frag).
    /// It selects the program and sets all of its uniforms itself. The lights come from
    /// the uniform buffer of game::LightRig, which must be filled for this frame and
    /// connected to the program.
    ///
    /// view and projection are the matrices of the frame. timeSeconds is the clock of
    /// the wind: any time that keeps growing. lit is false in the lighting mode Unlit,
    /// where the grass is shown at full brightness like everything else. viewMode
    /// selects the debug views, as for the walls.
    ///
    /// A blade is a flat strip that is seen from both sides. If back-face culling is
    /// switched on, it is switched off for this draw and on again afterwards.
    void draw(const gfx::Shader& shader, const glm::mat4& view, const glm::mat4& projection,
              const GrassSettings& settings, float timeSeconds, bool lit, ViewMode viewMode) const;

private:
    gfx::Mesh m_points;
    std::size_t m_tuftCount = 0;
};

} // namespace game
