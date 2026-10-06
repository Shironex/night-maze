#version 410 core
// Fragment shader of lit models, lighting per vertex (Gouraud shading): the light was
// computed in gouraud.vert, here it meets the texture and the two shadows.
// See docs/modules/renderer/lighting-gouraud-phong.md

// The shadow maps of the moon and of the flashlight and the functions moonShadow and
// flashlightShadow: the same file lit.frag includes.
#include "common/shadows.glsl"

// Inputs from the vertex shader: the light of the three vertices of the triangle,
// blended for this fragment.
in vec2 vUv;            // texture coordinate
in vec3 vDiffuseLight;  // ambient and diffuse light
in vec3 vSpecularLight; // highlight

// For the shadow of the moon, see gouraud.vert: the part of the two light values that
// comes from the moon, the position of this fragment and how much the surface faces
// the moon.
in vec3 vMoonDiffuseLight;
in vec3 vMoonSpecularLight;
in vec3 vWorldPosition;
in float vMoonFacing;

// The same for the shadow of the flashlight.
in vec3 vFlashlightDiffuseLight;
in vec3 vFlashlightSpecularLight;
in float vFlashlightFacing;

// The texture (the number of a texture unit) and the colour of the material, as in
// textured.frag.
uniform sampler2D uTexture;
uniform vec3 uTint;

// Light the surface gives off by itself (the glow of the crystals), as in lit.frag.
// Black for everything else.
uniform vec3 uEmissive;

// Output: the color written to the HDR framebuffer of the scene (red, green, blue,
// alpha), as a LINEAR colour that may be brighter than 1.
out vec4 fragColor;

void main() {
    // The same combination as in lit.frag: the colour of the surface times the diffuse
    // light, plus the highlight. The texture is still read per fragment, only the light
    // is per vertex. The glow of the surface itself joins the diffuse light, as in
    // lit.frag. All values are linear, and the result is encoded for the screen later,
    // in the composite pass (see lit.frag).
    //
    // The one thing computed per fragment is the shadow test, for the moon and for the
    // flashlight: whether this fragment lies in a shadow is looked up in the two
    // shadow maps here, and the share of the moon light and of the flashlight light it
    // loses is taken away from the blended light, as in lit.frag. The ambient light
    // and the crystal lights stay as they are.
    float shadow = moonShadow(vWorldPosition, vMoonFacing);
    float flashlightShade = flashlightShadow(vWorldPosition, vFlashlightFacing);
    vec3 diffuse = max(vDiffuseLight - vMoonDiffuseLight * shadow -
                           vFlashlightDiffuseLight * flashlightShade,
                       0.0);
    vec3 specular = max(vSpecularLight - vMoonSpecularLight * shadow -
                            vFlashlightSpecularLight * flashlightShade,
                        0.0);

    vec3 surface = texture(uTexture, vUv).rgb * uTint;
    fragColor = vec4(surface * (diffuse + uEmissive) + specular, 1.0);
}
