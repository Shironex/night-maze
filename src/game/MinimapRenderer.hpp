// MinimapRenderer: draws the minimap into a framebuffer of its own (offscreen rendering)
// and puts that picture into the middle of the window.
#pragma once

#include "core/Window.hpp"
#include "game/Minimap.hpp"
#include "gfx/Buffer.hpp"
#include "gfx/Framebuffer.hpp"
#include "gfx/VertexArray.hpp"

#include <glm/glm.hpp>

#include <span>

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

/// The OpenGL side of the minimap. What the map shows is plain data
/// (game::buildMinimapVertices). This class owns the framebuffer the map is drawn into,
/// the vertex buffer the triangles are copied into, and draws the two passes:
///
///   1. drawMap: the framebuffer of the minimap becomes the target, a small square
///      GL_RGBA8 texture. The triangles of the map are drawn into it with the minimap
///      program (post/minimap.vert and post/minimap.frag).
///   2. drawOverlay: the window becomes the target again, and that texture is drawn
///      into the middle of the window with the overlay program (post/composite.vert
///      and post/minimap_overlay.frag).
///
/// Both run AFTER the composite pass of game::PostProcess. The map must not pass through
/// it: the fog, the bloom, the exposure and the tone mapping are made for the scene and
/// would spoil a schematic. That also means that the composite pass does not encode the
/// map to sRGB. Nothing has to: the colours of the map are sRGB values from the start
/// and are written unchanged in both passes (see the two fragment shaders).
///
/// It is a class of its own and not a part of game::PostProcess, because that class
/// knows nothing about the game, and the map is made of game data.
///
/// It owns OpenGL objects, so it must be destroyed before the window.
class MinimapRenderer {
public:
    /// Creates the vertex array and an empty vertex buffer and describes the two
    /// attributes of game::MinimapVertex. The framebuffer is created by the first
    /// drawMap, when the size is known.
    MinimapRenderer();

    /// The offscreen pass. Makes the framebuffer of the minimap the target (a square of
    /// pixels by pixels, created again when pixels is not the size it has), clears it
    /// to MINIMAP_BACKGROUND_COLOR and draws vertices as triangles with shader, the
    /// minimap program. mapToClip is game::minimapProjection of the maze.
    ///
    /// The vertices are copied to the graphics card in every call: the list is built
    /// again in every frame, because the arrow of the player is part of it.
    ///
    /// Returns false when nothing was drawn: pixels is below 1, the program is not
    /// valid or the framebuffer could not be created (logged once per size). On true
    /// the framebuffer of the minimap is left bound, with the viewport at its size, the
    /// depth test and the blending switched off: drawOverlay has to follow.
    bool drawMap(const gfx::Shader& shader, std::span<const MinimapVertex> vertices,
                 const glm::mat4& mapToClip, int pixels);

    /// The pass that shows the map. Makes the window the target again and draws the
    /// picture of the minimap into the square rect (game::minimapRect) with shader, the
    /// overlay program, mixed with the scene by opacity (0 to 1). windowSize is the
    /// framebuffer size of the window in pixels.
    ///
    /// It leaves the window bound with the viewport over all of it, as the composite
    /// pass does, the depth test off and the blending as it found it. That holds also
    /// when it could not draw (no picture yet, or a program that is not valid).
    void drawOverlay(const gfx::Shader& shader, const MinimapRect& rect, float opacity,
                     core::Size windowSize) const;

    /// The framebuffer of the minimap, for the debug UI: its size, its format and its
    /// colour texture, which can be shown as it is. Not valid before the first drawMap
    /// that drew.
    const gfx::Framebuffer& target() const { return m_target; }

private:
    // The picture of the map: colour only, the map is flat and needs no depth test.
    gfx::Framebuffer m_target;
    // The size the last drawMap asked for. Kept apart from the size of m_target, so
    // that a creation that failed is not tried again in every frame.
    int m_requestedSize = 0;

    // Order matters: members are constructed top to bottom. The vertex array comes
    // first and its constructor binds it. The buffer is created next and stays bound to
    // GL_ARRAY_BUFFER, which is how the attribute setup in the constructor body tells
    // the vertex array which buffer to read from (the same order as in gfx::Mesh).
    gfx::VertexArray m_vertexArray;
    // The triangles of the map. Created with GL_DYNAMIC_DRAW and filled in every
    // drawMap.
    gfx::Buffer m_vertexBuffer;

    // A vertex array without attributes, for the triangle of the overlay pass. That
    // triangle has no vertex data (post/composite.vert computes its corners), but
    // a Core profile refuses to draw without a vertex array object bound.
    gfx::VertexArray m_triangle;
};

} // namespace game
