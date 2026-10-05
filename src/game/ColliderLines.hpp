// ColliderLines: draws collision boxes as thin lines, a debug view of the collisions.
// See docs/modules/scene/collision.md
#pragma once

#include "gfx/Mesh.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <span>

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

/// Draws axis-aligned boxes as their 12 edges.
///
/// It owns one small mesh: the edges of a cube with a side of 1. Every box is that cube,
/// scaled to the size of the box and moved to its place by the model matrix. It owns
/// OpenGL objects, so it must be destroyed before the window.
class ColliderLines {
public:
    /// Uploads the unit cube.
    ColliderLines();

    /// Draws every box in one colour. shader is the flat colour program (color.vert and
    /// color.frag): it must be in use, with uView and uProjection already set. The
    /// function sets uColor, and uModel for every box.
    void draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
              const glm::vec3& color) const;

private:
    gfx::Mesh m_unitCube;
};

} // namespace game
