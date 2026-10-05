// Tests of scene::normalMatrix: the matrix that takes normals to world space.
// See docs/modules/scene/transforms.md
#include "scene/Transform.hpp"

#include <doctest/doctest.h>

namespace {

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    CHECK(actual.x == doctest::Approx(expected.x).epsilon(0.0001));
    CHECK(actual.y == doctest::Approx(expected.y).epsilon(0.0001));
    CHECK(actual.z == doctest::Approx(expected.z).epsilon(0.0001));
}

// How a model matrix moves a direction that lies IN a surface (a tangent): w = 0 marks
// a direction, so the translation does not apply.
glm::vec3 transformDirection(const glm::mat4& matrix, const glm::vec3& direction) {
    return glm::vec3{matrix * glm::vec4{direction, 0.0F}};
}

} // namespace

TEST_CASE("the normal matrix of an object that is only moved changes no normal") {
    scene::Transform transform;
    transform.position = {5.0F, -2.0F, 9.0F};

    const glm::mat3 normalMatrix = scene::normalMatrix(transform.matrix());

    checkVector(normalMatrix * glm::vec3{0.0F, 1.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    checkVector(normalMatrix * glm::vec3{1.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F});
}

TEST_CASE("the normal matrix of a turned object turns the normal with it") {
    // A wall along Z: the wall model turned a quarter turn around Y. The normal of its
    // front face, +Z in the model, points along +X in the world.
    scene::Transform transform;
    transform.position = {4.0F, 0.0F, 3.0F};
    transform.rotationDegrees = {0.0F, 90.0F, 0.0F};
    const glm::mat4 model = transform.matrix();

    checkVector(scene::normalMatrix(model) * glm::vec3{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F});
    // Without scale the upper 3 x 3 part of the model matrix does the same.
    checkVector(glm::mat3(model) * glm::vec3{0.0F, 0.0F, 1.0F}, {1.0F, 0.0F, 0.0F});
}

TEST_CASE("with unequal scale only the normal matrix keeps a normal perpendicular") {
    // A slope: a surface that contains the direction (1, 1, 0), with the normal
    // (-1, 1, 0) / sqrt(2). The object is stretched 4 times along X only.
    const glm::vec3 tangent{1.0F, 1.0F, 0.0F};
    const glm::vec3 normal = glm::normalize(glm::vec3{-1.0F, 1.0F, 0.0F});
    REQUIRE(glm::dot(tangent, normal) == doctest::Approx(0.0F));

    scene::Transform transform;
    transform.scale = {4.0F, 1.0F, 1.0F};
    const glm::mat4 model = transform.matrix();

    // The surface after the stretch contains the direction (4, 1, 0).
    const glm::vec3 worldTangent = transformDirection(model, tangent);
    checkVector(worldTangent, {4.0F, 1.0F, 0.0F});

    // mat3(model) stretches the normal the same way, to (-4, 1, 0): it leans along the
    // surface instead of standing on it.
    const glm::vec3 wrongNormal = glm::mat3(model) * normal;
    CHECK(glm::dot(worldTangent, wrongNormal) != doctest::Approx(0.0F));

    // The normal matrix shrinks the x of the normal instead: (-1/4, 1, 0) is
    // perpendicular to (4, 1, 0).
    const glm::vec3 rightNormal = scene::normalMatrix(model) * normal;
    CHECK(glm::dot(worldTangent, rightNormal) == doctest::Approx(0.0F));
    checkVector(glm::normalize(rightNormal), glm::normalize(glm::vec3{-0.25F, 1.0F, 0.0F}));
}

TEST_CASE("with equal scale the normal matrix changes only the length of a normal") {
    scene::Transform transform;
    transform.scale = glm::vec3{2.0F};

    const glm::vec3 normal = scene::normalMatrix(transform.matrix()) * glm::vec3{0.0F, 1.0F, 0.0F};

    // The direction is kept. The length is 1 / 2: the shader normalizes.
    checkVector(normal, {0.0F, 0.5F, 0.0F});
}
