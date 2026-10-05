#version 410 core
// Fragment shader of lit models, lighting per vertex (Gouraud shading): the light was
// computed in gouraud.vert, here it only meets the texture.
// See docs/modules/renderer/lighting-gouraud-phong.md

// Inputs from the vertex shader: the light of the three vertices of the triangle,
// blended for this fragment.
in vec2 vUv;            // texture coordinate
in vec3 vDiffuseLight;  // ambient and diffuse light
in vec3 vSpecularLight; // highlight

// The texture (the number of a texture unit) and the colour of the material, as in
// textured.frag.
uniform sampler2D uTexture;
uniform vec3 uTint;

// Light the surface gives off by itself (the glow of the crystals), as in lit.frag.
// Black for everything else.
uniform vec3 uEmissive;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // The same combination as in lit.frag: the colour of the surface times the diffuse
    // light, plus the highlight. The texture is still read per fragment, only the light
    // is per vertex. The glow of the surface itself joins the diffuse light, as in
    // lit.frag. No gamma correction here either (see lit.frag).
    vec3 surface = texture(uTexture, vUv).rgb * uTint;
    fragColor = vec4(surface * (vDiffuseLight + uEmissive) + vSpecularLight, 1.0);
}
