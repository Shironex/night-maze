#version 410 core
// Fragment shader of lit models, lighting per fragment (Phong shading): the light is
// computed for every fragment from its own position and normal. The highlight formula
// (Phong or Blinn-Phong) is chosen by the uniform uSpecularModel.
// See docs/modules/renderer/lighting-gouraud-phong.md

// The light block and the function computeLighting. The same file is included by
// gouraud.vert.
#include "common/lighting.glsl"
// The normal map and the function surfaceNormal. The same file is included by
// textured.frag, for its debug view of the normals.
#include "common/normal_map.glsl"

// Inputs from the vertex shader, already blended for this fragment.
in vec2 vUv;            // texture coordinate
in vec3 vNormal;        // normal in world space, no longer exactly of length 1
in vec3 vTangent;       // tangent in world space, no longer exactly of length 1
in vec3 vWorldPosition; // position in world space

// The texture (the number of a texture unit) and the colour of the material, as in
// textured.frag.
uniform sampler2D uTexture;
uniform vec3 uTint;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // The normal of this fragment: the one of the model, or with normal mapping the one
    // read from the normal map. This is the only place where normal mapping enters the
    // lighting: the formulas of computeLighting do not know where their normal comes
    // from. It needs a normal per fragment, which is why the Gouraud program (light per
    // vertex) has no normal mapping.
    vec3 normal = surfaceNormal(vNormal, vTangent, vUv);
    Lighting lighting = computeLighting(vWorldPosition, normal);

    // The colour of the surface takes part in the diffuse light only: a red wall
    // reflects the red part of the light. The highlight is added on top in the colour
    // of the light, as in the Phong model of the lecture.
    //
    // No gamma correction in this milestone: the texture values are used as they are,
    // and the result is written as it is. Gamma (sRGB textures and an sRGB framebuffer)
    // arrives with the HDR pipeline in M7.
    vec3 surface = texture(uTexture, vUv).rgb * uTint;
    fragColor = vec4(surface * lighting.diffuse + lighting.specular, 1.0);
}
