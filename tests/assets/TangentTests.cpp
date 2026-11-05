// Tests of assets::triangleTangents, assets::computeTangents and
// assets::countMirroredTriangles.
#include "assets/Tangents.hpp"

#include <doctest/doctest.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace {

void checkVec3(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(0.001));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(0.001));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(0.001));
}

// True when no component is NaN or infinite.
bool isFinite(const glm::vec3& vector) {
    return std::isfinite(vector.x) && std::isfinite(vector.y) && std::isfinite(vector.z);
}

gfx::Vertex makeVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec2& uv) {
    gfx::Vertex vertex;
    vertex.position = position;
    vertex.normal = normal;
    vertex.uv = uv;
    return vertex;
}

constexpr glm::vec3 TOWARDS_VIEWER{0.0F, 0.0F, 1.0F};

// A square of 1 x 1 in the XY plane that faces +Z, like a piece of wall seen from the
// front. The texture lies on it the plain way: u grows to the right (+X), v grows up
// (+Y). Corners: bottom left, bottom right, top right, top left.
std::vector<gfx::Vertex> uprightSquare() {
    return {
        makeVertex({0.0F, 0.0F, 0.0F}, TOWARDS_VIEWER, {0.0F, 0.0F}),
        makeVertex({1.0F, 0.0F, 0.0F}, TOWARDS_VIEWER, {1.0F, 0.0F}),
        makeVertex({1.0F, 1.0F, 0.0F}, TOWARDS_VIEWER, {1.0F, 1.0F}),
        makeVertex({0.0F, 1.0F, 0.0F}, TOWARDS_VIEWER, {0.0F, 1.0F}),
    };
}

// The two triangles of a square, both counter clockwise.
constexpr std::array<std::uint32_t, 6> SQUARE_INDICES = {0, 1, 2, 0, 2, 3};

} // namespace

TEST_CASE("triangleTangents: the tangent is where u grows, the bitangent where v grows") {
    glm::vec3 tangent{0.0F};
    glm::vec3 bitangent{0.0F};

    SUBCASE("a texture that lies upright: u along +X, v along +Y") {
        REQUIRE(assets::triangleTangents({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F},
                                         {0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, tangent,
                                         bitangent));
        checkVec3(tangent, {1.0F, 0.0F, 0.0F});
        checkVec3(bitangent, {0.0F, 1.0F, 0.0F});
    }

    SUBCASE("the order of the corners does not matter") {
        REQUIRE(assets::triangleTangents({1.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                                         {1.0F, 1.0F}, {0.0F, 0.0F}, {1.0F, 0.0F}, tangent,
                                         bitangent));
        checkVec3(tangent, {1.0F, 0.0F, 0.0F});
        checkVec3(bitangent, {0.0F, 1.0F, 0.0F});
    }

    SUBCASE("a texture turned by a quarter: u along +Y, v along -X") {
        REQUIRE(assets::triangleTangents({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F},
                                         {0.0F, 0.0F}, {0.0F, -1.0F}, {1.0F, -1.0F}, tangent,
                                         bitangent));
        checkVec3(tangent, {0.0F, 1.0F, 0.0F});
        checkVec3(bitangent, {-1.0F, 0.0F, 0.0F});
    }

    SUBCASE("a texture that repeats twice per metre: the tangent is half as long") {
        // One step of 1 in u is only half a metre on the surface.
        REQUIRE(assets::triangleTangents({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F}, {1.0F, 1.0F, 0.0F},
                                         {0.0F, 0.0F}, {2.0F, 0.0F}, {2.0F, 4.0F}, tangent,
                                         bitangent));
        checkVec3(tangent, {0.5F, 0.0F, 0.0F});
        checkVec3(bitangent, {0.0F, 0.25F, 0.0F});
    }

    SUBCASE("a triangle anywhere in space: a floor with u along +X and v along -Z") {
        REQUIRE(assets::triangleTangents({5.0F, 2.0F, 3.0F}, {7.0F, 2.0F, 3.0F}, {7.0F, 2.0F, 1.0F},
                                         {0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, tangent,
                                         bitangent));
        checkVec3(tangent, {2.0F, 0.0F, 0.0F});
        checkVec3(bitangent, {0.0F, 0.0F, -2.0F});
    }
}

TEST_CASE("triangleTangents: degenerate UVs are refused and the outputs stay as they were") {
    glm::vec3 tangent{7.0F, 8.0F, 9.0F};
    glm::vec3 bitangent{1.0F, 2.0F, 3.0F};

    SUBCASE("all three corners have the same uv") {
        CHECK_FALSE(assets::triangleTangents({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                                             {1.0F, 1.0F, 0.0F}, {0.5F, 0.5F}, {0.5F, 0.5F},
                                             {0.5F, 0.5F}, tangent, bitangent));
    }

    SUBCASE("the three uvs lie on one line") {
        CHECK_FALSE(assets::triangleTangents({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F},
                                             {1.0F, 1.0F, 0.0F}, {0.0F, 0.0F}, {1.0F, 1.0F},
                                             {2.0F, 2.0F}, tangent, bitangent));
    }

    checkVec3(tangent, {7.0F, 8.0F, 9.0F});
    checkVec3(bitangent, {1.0F, 2.0F, 3.0F});
}

TEST_CASE("computeTangents: a square with an upright texture gets the tangent +X everywhere") {
    std::vector<gfx::Vertex> vertices = uprightSquare();

    assets::computeTangents(vertices, SQUARE_INDICES);

    for (const gfx::Vertex& vertex : vertices) {
        checkVec3(vertex.tangent, {1.0F, 0.0F, 0.0F});
        // The bitangent the shader builds, cross(N, T), is then "up": where v grows.
        checkVec3(glm::cross(vertex.normal, vertex.tangent), {0.0F, 1.0F, 0.0F});
    }
}

TEST_CASE("computeTangents: the tangent has length 1 whatever the size of the texture") {
    std::vector<gfx::Vertex> vertices = uprightSquare();
    for (gfx::Vertex& vertex : vertices) {
        // The square is 3 m wide and the texture repeats 8 times across it.
        vertex.position.x *= 3.0F;
        vertex.uv.x *= 8.0F;
    }

    assets::computeTangents(vertices, SQUARE_INDICES);

    for (const gfx::Vertex& vertex : vertices) {
        checkVec3(vertex.tangent, {1.0F, 0.0F, 0.0F});
    }
}

TEST_CASE("computeTangents: the tangent is made perpendicular to the normal (Gram-Schmidt)") {
    // The same square, but with a normal that leans towards +X, as on a smooth shaded
    // model. The tangent of the triangles is +X, which is not perpendicular to it.
    std::vector<gfx::Vertex> vertices = uprightSquare();
    const glm::vec3 leaningNormal{0.6F, 0.0F, 0.8F};
    for (gfx::Vertex& vertex : vertices) {
        vertex.normal = leaningNormal;
    }

    assets::computeTangents(vertices, SQUARE_INDICES);

    for (const gfx::Vertex& vertex : vertices) {
        // +X minus its part along the normal: (1, 0, 0) - 0.6 * (0.6, 0, 0.8) =
        // (0.64, 0, -0.48), and with length 1 that is (0.8, 0, -0.6).
        checkVec3(vertex.tangent, {0.8F, 0.0F, -0.6F});
        CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
    }
}

TEST_CASE("computeTangents: a vertex shared by two triangles gets the average tangent") {
    // Two triangles in the XY plane that share vertex 0. On the first one u grows along
    // +X, on the second one along +Y.
    std::vector<gfx::Vertex> vertices = {
        makeVertex({0.0F, 0.0F, 0.0F}, TOWARDS_VIEWER, {0.0F, 0.0F}),
        makeVertex({1.0F, 0.0F, 0.0F}, TOWARDS_VIEWER, {1.0F, 0.0F}),
        makeVertex({0.0F, 1.0F, 0.0F}, TOWARDS_VIEWER, {0.0F, 1.0F}),
        makeVertex({0.0F, -1.0F, 0.0F}, TOWARDS_VIEWER, {-1.0F, 0.0F}),
        makeVertex({-1.0F, 0.0F, 0.0F}, TOWARDS_VIEWER, {0.0F, 1.0F}),
    };
    const std::vector<std::uint32_t> indices = {0, 1, 2, 0, 3, 4};

    assets::computeTangents(vertices, indices);

    const float diagonal = std::sqrt(0.5F);
    checkVec3(vertices[0].tangent, {diagonal, diagonal, 0.0F});
    // The vertices that belong to one triangle only keep the tangent of that triangle.
    checkVec3(vertices[1].tangent, {1.0F, 0.0F, 0.0F});
    checkVec3(vertices[2].tangent, {1.0F, 0.0F, 0.0F});
    checkVec3(vertices[3].tangent, {0.0F, 1.0F, 0.0F});
    checkVec3(vertices[4].tangent, {0.0F, 1.0F, 0.0F});
}

TEST_CASE("computeTangents: degenerate input gives no NaN but some tangent in the surface") {
    SUBCASE("all uvs are the same: no direction of growing u exists") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            vertex.uv = {0.0F, 0.0F};
        }

        assets::computeTangents(vertices, SQUARE_INDICES);

        for (const gfx::Vertex& vertex : vertices) {
            CHECK(isFinite(vertex.tangent));
            CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
            CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        }
    }

    SUBCASE("a tangent parallel to the normal: nothing is left after Gram-Schmidt") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            vertex.normal = {1.0F, 0.0F, 0.0F};
        }

        assets::computeTangents(vertices, SQUARE_INDICES);

        for (const gfx::Vertex& vertex : vertices) {
            CHECK(isFinite(vertex.tangent));
            CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
            CHECK(glm::dot(vertex.tangent, vertex.normal) == doctest::Approx(0.0F));
        }
    }

    SUBCASE("a triangle without an area") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            vertex.position = {2.0F, 2.0F, 2.0F};
        }

        assets::computeTangents(vertices, SQUARE_INDICES);

        for (const gfx::Vertex& vertex : vertices) {
            CHECK(isFinite(vertex.tangent));
            CHECK(glm::length(vertex.tangent) == doctest::Approx(1.0F));
        }
    }

    SUBCASE("a vertex without a normal and without a triangle") {
        // The corners of the collision lines are like this.
        std::array<gfx::Vertex, 1> vertices{};

        assets::computeTangents(vertices, {});

        CHECK(isFinite(vertices[0].tangent));
        CHECK(glm::length(vertices[0].tangent) == doctest::Approx(1.0F));
    }
}

TEST_CASE("computeTangents: bad indices are skipped") {
    std::vector<gfx::Vertex> vertices = uprightSquare();
    // The first triangle is fine. The second names a vertex that does not exist, and
    // the last two indices do not make a triangle.
    const std::vector<std::uint32_t> indices = {0, 1, 2, 0, 2, 99, 1, 2};

    assets::computeTangents(vertices, indices);

    checkVec3(vertices[0].tangent, {1.0F, 0.0F, 0.0F});
    checkVec3(vertices[1].tangent, {1.0F, 0.0F, 0.0F});
    checkVec3(vertices[2].tangent, {1.0F, 0.0F, 0.0F});
    // Vertex 3 is only used by the skipped triangle: it gets the fallback tangent.
    CHECK(isFinite(vertices[3].tangent));
    CHECK(glm::length(vertices[3].tangent) == doctest::Approx(1.0F));
    CHECK(assets::countMirroredTriangles(vertices, indices) == 0U);
}

TEST_CASE("countMirroredTriangles: handedness of the tangent space") {
    SUBCASE("an upright texture is not mirrored") {
        const std::vector<gfx::Vertex> vertices = uprightSquare();
        CHECK(assets::countMirroredTriangles(vertices, SQUARE_INDICES) == 0U);
    }

    SUBCASE("a texture turned by a quarter is not mirrored either") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            // (u, v) becomes (-v, u): a rotation, not a mirror image.
            vertex.uv = {-vertex.uv.y, vertex.uv.x};
        }
        CHECK(assets::countMirroredTriangles(vertices, SQUARE_INDICES) == 0U);
    }

    SUBCASE("a texture with u flipped is mirrored on both triangles") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            vertex.uv.x = -vertex.uv.x;
        }
        CHECK(assets::countMirroredTriangles(vertices, SQUARE_INDICES) == 2U);

        // What goes wrong there: v still grows upwards (+Y), but the bitangent the
        // shader builds from the normal and the tangent points down.
        assets::computeTangents(vertices, SQUARE_INDICES);
        checkVec3(vertices[0].tangent, {-1.0F, 0.0F, 0.0F});
        checkVec3(glm::cross(vertices[0].normal, vertices[0].tangent), {0.0F, -1.0F, 0.0F});
    }

    SUBCASE("the same square seen from behind (normals turned) is mirrored") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            vertex.normal = -vertex.normal;
        }
        CHECK(assets::countMirroredTriangles(vertices, SQUARE_INDICES) == 2U);
    }

    SUBCASE("triangles with degenerate UVs are not counted") {
        std::vector<gfx::Vertex> vertices = uprightSquare();
        for (gfx::Vertex& vertex : vertices) {
            vertex.uv = {0.25F, 0.25F};
        }
        CHECK(assets::countMirroredTriangles(vertices, SQUARE_INDICES) == 0U);
    }
}
