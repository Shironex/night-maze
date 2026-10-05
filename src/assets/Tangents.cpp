// Tangents: computes the tangent of every vertex of a mesh from positions and UVs.
// See docs/modules/gfx/normal-mapping.md
#include "assets/Tangents.hpp"

#include <cmath>
#include <vector>

namespace assets {

namespace {

// A triangle is three indices.
constexpr std::size_t INDICES_PER_TRIANGLE = 3;

// Below this value the determinant of the UV differences counts as zero. The determinant
// is twice the area of the triangle in UV space. The smallest triangles of the models
// have about 0.01 there, far above this limit.
constexpr float MIN_UV_DETERMINANT = 1.0e-12F;

// A vector shorter than this cannot be brought to length 1 reliably: dividing by its
// length would blow up rounding errors, and dividing by 0 gives NaN.
constexpr float MIN_LENGTH = 1.0e-6F;

constexpr glm::vec3 X_AXIS{1.0F, 0.0F, 0.0F};
constexpr glm::vec3 Y_AXIS{0.0F, 1.0F, 0.0F};
constexpr glm::vec3 Z_AXIS{0.0F, 0.0F, 1.0F};

// The vector with length 1, or all zeros when it is too short to have a direction.
glm::vec3 normalizedOrZero(const glm::vec3& vector) {
    const float length = glm::length(vector);
    if (length < MIN_LENGTH) {
        return glm::vec3{0.0F};
    }
    return vector / length;
}

// The coordinate axis that is furthest from the direction of normal: the one along which
// normal has its smallest component. It is never parallel to normal.
glm::vec3 leastAlignedAxis(const glm::vec3& normal) {
    const glm::vec3 size = glm::abs(normal);
    if (size.x <= size.y && size.x <= size.z) {
        return X_AXIS;
    }
    return size.y <= size.z ? Y_AXIS : Z_AXIS;
}

// Gram-Schmidt: the part of tangent that is perpendicular to normal, with length 1.
// dot(normal, tangent) is how far tangent reaches along the normal. Taking that much of
// the normal away leaves a vector that lies in the surface.
glm::vec3 orthonormalTangent(const glm::vec3& normal, const glm::vec3& tangent) {
    // The formula needs a normal of length 1. A vertex without a normal (all zeros)
    // stays at zero here, and then nothing is taken away from the tangent.
    const glm::vec3 unitNormal = normalizedOrZero(normal);

    glm::vec3 inSurface = tangent - unitNormal * glm::dot(unitNormal, tangent);
    if (glm::length(inSurface) < MIN_LENGTH) {
        // No usable tangent. Any direction in the surface is better than zeros or NaN:
        // start from an axis that is surely not parallel to the normal.
        const glm::vec3 axis = leastAlignedAxis(unitNormal);
        inSurface = axis - unitNormal * glm::dot(unitNormal, axis);
    }
    return glm::normalize(inSurface);
}

// True when the three indices of the triangle that starts at indices[first] exist and
// point at existing vertices.
bool isTriangle(std::span<const std::uint32_t> indices, std::size_t first,
                std::size_t vertexCount) {
    if (first + INDICES_PER_TRIANGLE > indices.size()) {
        return false;
    }
    return indices[first] < vertexCount && indices[first + 1] < vertexCount &&
           indices[first + 2] < vertexCount;
}

} // namespace

bool triangleTangents(const glm::vec3& position0, const glm::vec3& position1,
                      const glm::vec3& position2, const glm::vec2& uv0, const glm::vec2& uv1,
                      const glm::vec2& uv2, glm::vec3& tangent, glm::vec3& bitangent) {
    const glm::vec3 edge1 = position1 - position0;
    const glm::vec3 edge2 = position2 - position0;
    // x is the difference in u, y the difference in v.
    const glm::vec2 deltaUv1 = uv1 - uv0;
    const glm::vec2 deltaUv2 = uv2 - uv0;

    const float determinant = deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y;
    if (std::abs(determinant) < MIN_UV_DETERMINANT) {
        return false;
    }

    tangent = (deltaUv2.y * edge1 - deltaUv1.y * edge2) / determinant;
    bitangent = (deltaUv1.x * edge2 - deltaUv2.x * edge1) / determinant;
    return true;
}

void computeTangents(std::span<gfx::Vertex> vertices, std::span<const std::uint32_t> indices) {
    // The sum of the tangents of all triangles that use a vertex, one entry per vertex.
    std::vector<glm::vec3> sums(vertices.size(), glm::vec3{0.0F});

    for (std::size_t first = 0; first < indices.size(); first += INDICES_PER_TRIANGLE) {
        if (!isTriangle(indices, first, vertices.size())) {
            continue;
        }
        const std::uint32_t index0 = indices[first];
        const std::uint32_t index1 = indices[first + 1];
        const std::uint32_t index2 = indices[first + 2];

        glm::vec3 tangent{0.0F};
        glm::vec3 bitangent{0.0F};
        if (!triangleTangents(vertices[index0].position, vertices[index1].position,
                              vertices[index2].position, vertices[index0].uv, vertices[index1].uv,
                              vertices[index2].uv, tangent, bitangent)) {
            continue;
        }

        // Length 1 before adding, so that a triangle with a stretched texture (a long
        // tangent) does not count more than its neighbours. Only the direction matters.
        const glm::vec3 direction = normalizedOrZero(tangent);
        sums[index0] += direction;
        sums[index1] += direction;
        sums[index2] += direction;
    }

    // The direction of a sum of vectors is the direction of their average, so the sum is
    // not divided by the number of triangles: orthonormalTangent sets the length anyway.
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        vertices[i].tangent = orthonormalTangent(vertices[i].normal, sums[i]);
    }
}

std::size_t countMirroredTriangles(std::span<const gfx::Vertex> vertices,
                                   std::span<const std::uint32_t> indices) {
    std::size_t mirrored = 0;

    for (std::size_t first = 0; first < indices.size(); first += INDICES_PER_TRIANGLE) {
        if (!isTriangle(indices, first, vertices.size())) {
            continue;
        }
        const gfx::Vertex& vertex0 = vertices[indices[first]];
        const gfx::Vertex& vertex1 = vertices[indices[first + 1]];
        const gfx::Vertex& vertex2 = vertices[indices[first + 2]];

        glm::vec3 tangent{0.0F};
        glm::vec3 bitangent{0.0F};
        if (!triangleTangents(vertex0.position, vertex1.position, vertex2.position, vertex0.uv,
                              vertex1.uv, vertex2.uv, tangent, bitangent)) {
            continue;
        }

        // The side the triangle faces, as the shader sees it: from the normals of its
        // corners. cross(N, T) is the bitangent the shader builds. When the real
        // bitangent points to the other side of the tangent (a negative dot product),
        // the texture is mirrored on this triangle.
        const glm::vec3 normal = vertex0.normal + vertex1.normal + vertex2.normal;
        if (glm::dot(glm::cross(normal, tangent), bitangent) < 0.0F) {
            ++mirrored;
        }
    }
    return mirrored;
}

} // namespace assets
