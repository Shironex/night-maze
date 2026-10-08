#version 410 core
// Vertex shader of the surfaces that show the sky (crystals and puddles): places the
// vertex on the screen and passes what the fragment shader needs to compute the light
// and the direction of the reflected ray.

// It does the same work as lit.vert: the environment mapping happens in the fragment
// shader, and what it needs from here (the position and the normal in world space) is
// exactly what the lighting per fragment needs too.

// Inputs: the four attributes of gfx::Vertex, as in lit.vert.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the model
layout(location = 1) in vec3 aNormal;   // direction the surface faces, length 1
layout(location = 2) in vec2 aUv;       // texture coordinate (u, v)
layout(location = 3) in vec3 aTangent;  // direction on the surface in which u grows, length 1

// Uniforms: set from C++. uModel and uNormalMatrix change with every object, uView and
// uProjection are the same for the whole frame.
uniform mat4 uModel;      // local space to world space
uniform mat4 uView;       // world space to view space
uniform mat4 uProjection; // view space to clip space
// Local space to world space for normals: the inverse transpose of the upper left 3 x 3
// part of uModel, computed in C++ (scene::normalMatrix). A crystal is turned and made
// bigger by its model matrix. The puddles are in world space already (their model
// matrix is the identity) and their normal is straight up at every vertex: the water
// lies on the uneven ground, but it mirrors like level water.
uniform mat3 uNormalMatrix;

// Outputs to the fragment shader, blended across the triangle by the rasterizer.
out vec2 vUv;            // texture coordinate
out vec3 vNormal;        // normal in world space
out vec3 vTangent;       // tangent in world space
out vec3 vWorldPosition; // position in world space

void main() {
    // Everything is computed in world space. The cube map of the sky is fixed to the
    // world (the direction (0, 1, 0) is always straight up), so the ray that is mirrored
    // in the fragment shader has to be a direction of the world as well. The lights and
    // the camera position in the light block are in world space too.
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vNormal = uNormalMatrix * aNormal;
    // The tangent lies IN the surface, so it turns and stretches with the model:
    // mat3(uModel), the model matrix without its translation (see lit.vert).
    vTangent = mat3(uModel) * aTangent;
    vUv = aUv;

    // World space, view space, clip space.
    gl_Position = uProjection * uView * worldPosition;
}
