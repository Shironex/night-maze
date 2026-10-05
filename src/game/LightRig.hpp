// LightRig: sends the lights of a frame to the graphics card and draws the light markers.
// See docs/modules/game/flashlight.md
#pragma once

#include "gfx/Mesh.hpp"
#include "gfx/UniformBuffer.hpp"

#include <glm/glm.hpp>

#include <span>

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct LightSet;
} // namespace scene

namespace game {

/// The OpenGL side of the lighting. The lights themselves are plain data
/// (scene::LightSet, built by game::buildLightSet). This class owns the uniform buffer
/// they are copied into, and a small cube that marks where a point light hangs.
///
/// It owns OpenGL objects, so it must be destroyed before the window.
class LightRig {
public:
    /// Creates the uniform buffer (the size of scene::LightBlockData, attached to
    /// LIGHT_BLOCK_BINDING_POINT) and uploads the marker cube.
    LightRig();

    /// Tells a lit program to read its block LightBlock from the buffer of this object.
    /// Needed once per program: the Shader repeats it after every reload. A program
    /// without the block is left alone.
    void connect(gfx::Shader& shader) const;

    /// Copies the lights to the uniform buffer. Call it once per frame, before the lit
    /// programs draw. cameraPosition is the eye the frame is drawn from, in world space.
    void upload(const scene::LightSet& lights, const glm::vec3& cameraPosition) const;

    /// Draws a small cube at every position, in one flat colour: the visible source of
    /// a point light. A light itself is not a thing that can be seen, only what it
    /// shines on. shader is the flat colour program (color.vert and color.frag): it must
    /// be in use, with uView and uProjection already set. The function sets uColor, and
    /// uModel for every cube. The cubes are not lit, so they glow in the dark.
    void drawMarkers(const gfx::Shader& shader, std::span<const glm::vec3> positions,
                     const glm::vec3& color) const;

private:
    gfx::UniformBuffer m_lightBuffer;
    gfx::Mesh m_markerCube;
};

} // namespace game
