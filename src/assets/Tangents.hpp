// Tangents: computes the tangent of every vertex of a mesh from positions and UVs.
// See docs/modules/gfx/normal-mapping.md
#pragma once

#include "gfx/Vertex.hpp"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <span>

namespace assets {

// A normal map stores directions in tangent space: +X is the way the texture coordinate u
// grows on the surface (the tangent T), +Y the way v grows (the bitangent B) and +Z the
// normal N. To use such a direction the shader needs T, B and N of the surface in the
// space of the model. N comes from the model file. T is computed here. B is not stored:
// the shader builds it as cross(N, T).
//
// The functions are plain math without OpenGL, so tests can run them.

/// The tangent and the bitangent of one triangle: the directions on the surface in which
/// u and v grow, scaled so that one step of 1 in u (or v) is one step of tangent (or
/// bitangent). Neither has length 1 in general.
///
/// position0..2 are the corners, uv0..2 their texture coordinates. With the two edges
/// E1 = position1 - position0 and E2 = position2 - position0 and the UV differences
/// (du1, dv1) = uv1 - uv0 and (du2, dv2) = uv2 - uv0, both edges can be written with the
/// two unknown directions:
///
///     E1 = du1 * T + dv1 * B
///     E2 = du2 * T + dv2 * B
///
/// Two equations, two unknowns. With det = du1 * dv2 - du2 * dv1 the solution is
///
///     T = (dv2 * E1 - dv1 * E2) / det
///     B = (du1 * E2 - du2 * E1) / det
///
/// Returns false and leaves the outputs unchanged when det is (nearly) zero: the three
/// UVs then lie on one line or in one point, and no direction of growing u exists.
bool triangleTangents(const glm::vec3& position0, const glm::vec3& position1,
                      const glm::vec3& position2, const glm::vec2& uv0, const glm::vec2& uv1,
                      const glm::vec2& uv2, glm::vec3& tangent, glm::vec3& bitangent);

/// Fills in the tangent of every vertex. Every three indices are one triangle.
///
/// A vertex used by several triangles gets the average of their tangents (each triangle
/// counts the same, whatever its size). The average is then made perpendicular to the
/// normal of the vertex and brought to length 1 (Gram-Schmidt), so the shader gets
/// a clean pair. The result never contains NaN: a triangle with degenerate UVs adds
/// nothing, and a vertex that got no usable tangent at all (only such triangles, no
/// triangle, or a tangent parallel to its normal) gets some direction perpendicular to
/// its normal instead. A normal map then still lights such a vertex, only with the
/// relief turned by an unknown angle.
///
/// A triangle with an index that is not smaller than vertices.size() is skipped.
void computeTangents(std::span<gfx::Vertex> vertices, std::span<const std::uint32_t> indices);

/// The number of triangles whose texture is mirrored: seen from the side the normals
/// point to, u and v turn the other way round than on a picture (v to the right of u
/// instead of to the left). For such a triangle cross(N, T) points against the real
/// bitangent, and a shader that builds the bitangent this way shows the relief of
/// a normal map upside down. A mesh with mirrored triangles needs a sign per vertex (the
/// "handedness", usually stored as a fourth tangent component). The models of the game
/// have none, which a test checks, so no sign is stored.
///
/// Triangles with degenerate UVs or a bad index are not counted.
std::size_t countMirroredTriangles(std::span<const gfx::Vertex> vertices,
                                   std::span<const std::uint32_t> indices);

} // namespace assets
