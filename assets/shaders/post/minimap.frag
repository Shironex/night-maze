#version 410 core
// Fragment shader of the minimap: writes the colour of a shape of the map into the
// picture of the map.
// See docs/modules/renderer/minimap.md

// Input from minimap.vert: the colour of the shape this pixel belongs to.
in vec3 vColor;

// Output: the color written to the texture of the minimap (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // The colour is written AS IT IS. The colours of the map are sRGB values (the
    // constants in game/Minimap.hpp): the numbers the screen is meant to get. Nothing
    // on the map is lit, so there is no linear math to do, and the picture of the map
    // never passes through post/composite.frag, the place that encodes the scene. The
    // texture is a plain GL_RGBA8 one, which stores the numbers unchanged, and
    // post/minimap_overlay.frag copies them to the window unchanged. So nothing here
    // calls srgbToLinear or linearToSrgb. Alpha 1: the picture itself is opaque.
    fragColor = vec4(vColor, 1.0);
}
