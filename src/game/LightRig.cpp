// LightRig: sends the lights of a frame to the graphics card.
// See docs/modules/game/flashlight.md
#include "game/LightRig.hpp"

#include "game/ShaderUniforms.hpp"
#include "gfx/Shader.hpp"
#include "scene/Light.hpp"
#include "scene/LightBlock.hpp"

namespace game {

LightRig::LightRig() : m_lightBuffer(sizeof(scene::LightBlockData), LIGHT_BLOCK_BINDING_POINT) {}

void LightRig::connect(gfx::Shader& shader) const {
    shader.bindUniformBlock(LIGHT_BLOCK_NAME, m_lightBuffer.bindingPoint(),
                            m_lightBuffer.sizeInBytes());
}

void LightRig::upload(const scene::LightSet& lights, const glm::vec3& cameraPosition) const {
    // The struct has exactly the bytes the block of the shader expects (the asserts in
    // scene/LightBlock.hpp), so it is copied as it is.
    const scene::LightBlockData block = scene::packLightBlock(lights, cameraPosition);
    m_lightBuffer.update(&block, sizeof(block));
}

} // namespace game
