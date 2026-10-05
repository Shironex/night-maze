#version 410 core
// Vertex shader of lit models, lighting per fragment (Phong shading): places the vertex
// on the screen and passes what the fragment shader needs to compute the light.
// See docs/modules/renderer/lighting-gouraud-phong.md

// Inputs: the four attributes of gfx::Vertex, as in textured.vert.
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
// part of uModel, computed in C++ (scene::normalMatrix). mat3(uModel) would be right
// only while no object is scaled differently along its axes.
uniform mat3 uNormalMatrix;

// Outputs to the fragment shader, blended across the triangle by the rasterizer.
out vec2 vUv;            // texture coordinate
out vec3 vNormal;        // normal in world space
out vec3 vTangent;       // tangent in world space
out vec3 vWorldPosition; // position in world space

void main() {
    // The lighting is computed in world space: the lights and the camera position in
    // the light block are in world space too.
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    vWorldPosition = worldPosition.xyz;
    vNormal = uNormalMatrix * aNormal;
    // The tangent lies IN the surface, like an edge of a triangle, so it turns and
    // stretches with the model: mat3(uModel), the model matrix without its translation.
    // The normal matrix is for directions that must stay perpendicular to the surface.
    // With these two matrices the pair stays perpendicular under any model matrix.
    vTangent = mat3(uModel) * aTangent;
    vUv = aUv;

    // World space, view space, clip space.
    gl_Position = uProjection * uView * worldPosition;
}
