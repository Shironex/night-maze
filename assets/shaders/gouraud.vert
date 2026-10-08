#version 410 core
// Vertex shader of lit models, lighting per vertex (Gouraud shading): the light is
// computed here, once for every vertex, and the fragment shader only blends the result.

// The light block and the function computeLighting: the very same file lit.frag
// includes. Only the place where the function is called differs.
#include "common/lighting.glsl"

// Inputs: three of the four attributes of gfx::Vertex. The tangent (location 3) is not
// read: it is only needed for normal mapping, and there is none here. A normal map holds
// one normal per texel, so it can only change light that is computed per fragment. This
// program computes the light at the vertices, 4 per wall face, and a texel between them
// has no way to take part. That is one more thing Gouraud shading cannot show.
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

// Outputs for the shadow of the moon. The LIGHT stays per vertex, but whether a point
// lies in a shadow is asked per fragment in gouraud.frag: the edge of a shadow runs
// across a wall face wherever it likes, and a wall face has only four vertices. Asked
// per vertex, a face would be shaded as a whole or blended from corner to corner, and
// the shadows would not look like shadows. So this shader hands over the share of the
// moon in the two light values above, the position the fragment shader looks up in the
// shadow map, and how much the surface faces the moon (for the bias).
out vec3 vMoonDiffuseLight;  // the part of vDiffuseLight that comes from the moon
out vec3 vMoonSpecularLight; // the part of vSpecularLight that comes from the moon
out vec3 vWorldPosition;     // position in world space
out float vMoonFacing;       // cosine between the normal and the direction to the moon

// The same three for the shadow of the flashlight, which has a shadow map of its own:
// its share of the two light values, and how much the surface faces it. The position
// above serves both maps.
out vec3 vFlashlightDiffuseLight;  // the part of vDiffuseLight from the flashlight
out vec3 vFlashlightSpecularLight; // the part of vSpecularLight from the flashlight
out float vFlashlightFacing;       // cosine between the normal and the way to the flashlight

void main() {
    vec4 worldPosition = uModel * vec4(aPosition, 1.0);
    // The normal of a vertex comes straight from the model with length 1, but the
    // normal matrix may change that length (scale), so it is normalized.
    vec3 normal = normalize(uNormalMatrix * aNormal);

    Lighting lighting = computeLighting(worldPosition.xyz, normal);
    vDiffuseLight = lighting.diffuse;
    vSpecularLight = lighting.specular;
    vMoonDiffuseLight = lighting.moonDiffuse;
    vMoonSpecularLight = lighting.moonSpecular;
    vWorldPosition = worldPosition.xyz;
    vMoonFacing = moonFacing(normal);
    vFlashlightDiffuseLight = lighting.flashlightDiffuse;
    vFlashlightSpecularLight = lighting.flashlightSpecular;
    vFlashlightFacing = flashlightFacing(normal, worldPosition.xyz);
    vUv = aUv;

    gl_Position = uProjection * uView * worldPosition;
}
