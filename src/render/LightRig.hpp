// LightRig: sends the lights of a frame to the graphics card.
#pragma once

#include "gfx/UniformBuffer.hpp"

#include <glm/glm.hpp>

namespace gfx {
class Shader;
} // namespace gfx

namespace scene {
struct LightSet;
} // namespace scene

namespace game {

/// The OpenGL side of the lighting. The lights themselves are plain data
/// (scene::LightSet, built by game::buildLightSet). This class owns the uniform buffer
/// they are copied into. A light itself is not a thing that can be seen, only what it
/// shines on: the visible source of every point light is a crystal, drawn by
/// game::GameplayRenderer.
///
/// It owns an OpenGL object, so it must be destroyed before the window.
class LightRig {
public:
    /// Creates the uniform buffer (the size of scene::LightBlockData, attached to
    /// LIGHT_BLOCK_BINDING_POINT).
    LightRig();

    /// Tells a lit program to read its block LightBlock from the buffer of this object.
    /// Needed once per program: the Shader repeats it after every reload. A program
    /// without the block is left alone.
    void connect(gfx::Shader& shader) const;

    /// Copies the lights to the uniform buffer. Call it once per frame, before the lit
    /// programs draw. cameraPosition is the eye the frame is drawn from, in world space.
    void upload(const scene::LightSet& lights, const glm::vec3& cameraPosition) const;

private:
    gfx::UniformBuffer m_lightBuffer;
};

} // namespace game
