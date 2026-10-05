#version 410 core
// Fragment shader of the grass: a colour that runs from dark at the root to light at the
// tip, lit by the lights of the scene.
// See docs/modules/renderer/grass-geometry.md

// The light block and the function computeLighting: the same file lit.frag includes, so
// the grass is lit by the very lights that light the walls.
#include "common/lighting.glsl"
// srgbToLinear, for the colours written in this file.
#include "common/color.glsl"

// Inputs from the geometry shader, already blended for this fragment.
in vec3 gWorldPosition; // position in world space
in vec2 gBladeUv;       // x: across the blade, y: 0 at the root and 1 at the tip

// Whether the grass is lit (1) or shown at full brightness (0). It is off in the lighting
// mode Unlit, where nothing in the scene is lit.
uniform bool uLit;

// What to show. The numbers are the values of game::ViewMode in C++, as in textured.frag.
//   0: the grass in its colours
//   1: the normal used for the lighting as a colour
//   2: the blade coordinate as a colour
uniform int uViewMode;

// Output: the color written to the HDR framebuffer of the scene (red, green, blue,
// alpha), as a LINEAR colour.
out vec4 fragColor;

// The two ends of the colour gradient. No texture is needed: a blade is too thin on the
// screen for a picture to show. The numbers are sRGB values, chosen by eye on a screen
// like the pixels of a texture, so main converts the colour to linear before the
// lighting. They are fairly light: the night comes from the lighting.
const vec3 ROOT_COLOR = vec3(0.10, 0.20, 0.07);
const vec3 TIP_COLOR = vec3(0.46, 0.64, 0.26);

// The normal the whole grass is lit with: straight up, like the ground it grows on. The
// real normal of a blade points sideways, and with it a blade would be bright from one
// side and black from the other, flickering as the camera walks past. With the normal of
// the ground a tuft is as bright as the ground around it.
const vec3 GRASS_NORMAL = vec3(0.0, 1.0, 0.0);

void main() {
    if (uViewMode == 1) {
        // The same colour coding as in textured.frag: a normal that points up is green.
        // Data shown as a colour: converted so that the encoding of the composite pass
        // gives back these numbers (see textured.frag).
        fragColor = vec4(srgbToLinear(GRASS_NORMAL * 0.5 + 0.5), 1.0);
        return;
    }
    if (uViewMode == 2) {
        // Across the blade goes to red and the height to green, like u and v of a model.
        fragColor = vec4(srgbToLinear(vec3(gBladeUv, 0.0)), 1.0);
        return;
    }

    // The gradient is blended between the sRGB numbers, the way it was chosen, and the
    // result is converted once: the same as reading it from an sRGB texture.
    vec3 color = srgbToLinear(mix(ROOT_COLOR, TIP_COLOR, gBladeUv.y));

    // Only the diffuse part of the lighting is used: ambient light plus the moon, the
    // crystals and the flashlight, each weaker with distance. Grass is not shiny, so the
    // highlight is left out.
    vec3 light = vec3(1.0);
    if (uLit) {
        light = computeLighting(gWorldPosition, GRASS_NORMAL).diffuse;
    }
    fragColor = vec4(color * light, 1.0);
}
