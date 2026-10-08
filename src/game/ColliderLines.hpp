// ColliderLines: draws collision boxes, spheres and single lines as thin lines, a debug
// view of the collisions and of the picking ray.
#pragma once

#include "gfx/Mesh.hpp"
#include "scene/Collider.hpp"

#include <glm/glm.hpp>

#include <span>

namespace gfx {
class Shader;
} // namespace gfx

namespace game {

/// Draws axis-aligned boxes as their 12 edges and spheres as three circles.
///
/// It owns three small meshes: the edges of a cube with a side of 1, a circle with
/// a radius of 1 and a line of length 1. Every box is that cube, scaled to the size of
/// the box and moved to its place by the model matrix. Every sphere is that circle drawn
/// three times, once around each axis. A line between two points is the unit line laid
/// between them. It owns OpenGL objects, so it must be destroyed before the window.
class ColliderLines {
public:
    /// Uploads the unit cube and the unit circle.
    ColliderLines();

    /// Draws every box in one colour. shader is the flat colour program (color.vert and
    /// color.frag): it must be in use, with uView and uProjection already set. The
    /// function sets uColor, and uModel for every box.
    void draw(const gfx::Shader& shader, std::span<const scene::Aabb> boxes,
              const glm::vec3& color) const;

    /// Draws every sphere in one colour, as three circles around its centre: one lying
    /// flat (in the XZ plane) and two standing upright (in the XY and the YZ plane).
    /// Three circles are enough to read the size and the place of a sphere, and they
    /// are the same picture from every side. shader is prepared as for draw.
    void drawSpheres(const gfx::Shader& shader, std::span<const scene::Sphere> spheres,
                     const glm::vec3& color) const;

    /// Draws one straight line from one point of the world to another, in one colour:
    /// the picking ray. shader is prepared as for draw.
    void drawLine(const gfx::Shader& shader, const glm::vec3& from, const glm::vec3& to,
                  const glm::vec3& color) const;

private:
    gfx::Mesh m_unitCube;
    gfx::Mesh m_unitCircle;
    gfx::Mesh m_unitLine;
};

} // namespace game
