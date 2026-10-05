#version 410 core
// Vertex shader of lit models, lighting per vertex (Gouraud shading): the light is
// computed here, once for every vertex, and the fragment shader only blends the result.
// See docs/modules/renderer/lighting-gouraud-phong.md

// The light block and the function computeLighting: the very same file lit.frag
// includes. Only the place where the function is called differs.
#include "common/lighting.glsl"

// Inputs: the three attributes of gfx::Vertex, as in textured.vert.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the model
layout(location = 1) in vec3 aNormal;   // direction the surface faces, length 1
layout(location = 2) in vec2 aUv;       // texture coordinate (u, v)

// Uniforms: the same four as in lit.vert.
uniform mat4 uModel;        // local space to world space
uniform mat4 uView;         // world space to view space
uniform mat4 uProjection;   // view space to clip space
uniform mat3 uNormalMatrix; // local space to world space for normals

// Outputs to the fragment shader. The rasterizer blends the two light values in a
// straight line between the three vertices of a triangle. That is the weakness of this
// method: light that falls between the vertices (the round spot of the flashlight on
// a large wall, a small highlight) is not at any vertex and so does not appear at all,
// or appears as a blurred triangle.
out vec2 vUv;            // texture coordinate
out vec3 vDiffuseLight;  // ambient and diffuse light at this vertex
out vec3 vSpecularLight; // highlight at this vertex

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    // The normal of a vertex comes straight from the model with length 1, but the
    // normal matrix may change that length (scale), so it is normalized.
    vec3 normal = normalize(uNormalMatrix * aNormal);

    Lighting lighting = computeLighting(worldPosition.xyz, normal);
    vDiffuseLight = lighting.diffuse;
    vSpecularLight = lighting.specular;
    vUv = aUv;

    gl_Position = uProjection * uView * worldPosition;
}
