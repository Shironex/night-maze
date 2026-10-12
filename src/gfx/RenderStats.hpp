// RenderStats: counters of what the game asks of the graphics card in one frame.
#pragma once

#include <glad/gl.h>

#include <cstddef>
#include <cstdint>

namespace gfx {

/// What the game has asked of OpenGL since the counters were last reset: once per frame.
///
/// Only the calls of the game itself are counted, at the places in src/gfx and src/game
/// that make them. Dear ImGui and RmlUi draw with their own OpenGL code and are not in
/// these numbers.
struct RenderCounters {
    /// Draw calls: glDrawElements and glDrawArrays.
    std::uint32_t drawCalls = 0;
    /// Triangles those draw calls handed to the card (trianglesOf). The blades the
    /// geometry shader of the grass makes on the card are not in it.
    std::uint32_t triangles = 0;
    /// Lookups of a uniform by its name (glGetUniformLocation). gfx::Shader remembers
    /// the answers, so only the first use of a name with a program is one: after the
    /// first frames the number is 0 until a shader is reloaded.
    std::uint32_t uniformLookups = 0;
    /// Textures bound to a texture unit for a shader to read: the bind functions of
    /// Texture2D, Cubemap, FrameTexture and Framebuffer.
    std::uint32_t textureBinds = 0;
};

/// How many triangles a draw call of count vertices (or indices) hands to the card.
/// With GL_TRIANGLES every three are one triangle. A draw call of lines or of points
/// has none.
std::uint32_t trianglesOf(GLenum primitive, std::size_t count);

/// What was counted between two readings: now minus start, field by field. start has
/// to be the earlier reading of the same frame.
RenderCounters countersSince(const RenderCounters& now, const RenderCounters& start);

// The counters themselves are one object for the whole program, hidden in
// RenderStats.cpp: a draw call is made deep inside the renderers, and handing every one
// of them a counter to fill would change every draw function for a number that is only
// shown. The game draws from one thread, so nothing has to be locked.

/// The counters since the last resetRenderCounters().
const RenderCounters& renderCounters();

/// Sets every counter to 0. The game calls it once, at the start of a frame.
void resetRenderCounters();

/// Counts one draw call of count vertices (or indices) of the given primitive. Called
/// next to every glDrawElements and glDrawArrays of the game.
void countDraw(GLenum primitive, std::size_t count);

/// Counts one glGetUniformLocation.
void countUniformLookup();

/// Counts one texture bound for reading.
void countTextureBind();

} // namespace gfx
