// Transform: position, rotation and scale of an object, turned into a model matrix.
#include "scene/Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace scene {

namespace {

// The three coordinate axes, used as rotation axes.
constexpr glm::vec3 AXIS_X{1.0F, 0.0F, 0.0F};
constexpr glm::vec3 AXIS_Y{0.0F, 1.0F, 0.0F};
constexpr glm::vec3 AXIS_Z{0.0F, 0.0F, 1.0F};

} // namespace

glm::mat4 Transform::matrix() const {
    // Start from the identity matrix ("change nothing"). Each glm function below
    // multiplies its matrix on the right side, so the last call is the first one applied
    // to a vertex: the lines read top to bottom, the vertex is transformed bottom to top.
    glm::mat4 model(1.0F);
    model = glm::translate(model, position);
    // GLM takes angles in radians.
    model = glm::rotate(model, glm::radians(rotationDegrees.y), AXIS_Y);
    model = glm::rotate(model, glm::radians(rotationDegrees.x), AXIS_X);
    model = glm::rotate(model, glm::radians(rotationDegrees.z), AXIS_Z);
    model = glm::scale(model, scale);
    return model;
}

glm::mat3 normalMatrix(const glm::mat4& modelMatrix) {
    // glm::mat3(mat4) keeps the upper left 3 x 3 part: rotation and scale, without the
    // translation in the fourth column.
    return glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
}

} // namespace scene
