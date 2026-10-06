#version 410 core
// Fragment shader that puts the picture of the minimap on the screen, in its corner of
// the window. Used with post/composite.vert.
// See docs/modules/renderer/minimap.md

// Input from composite.vert: the texture coordinate of this pixel, (0, 0) in the bottom
// left corner of the area drawn into and (1, 1) in the top right one. The area is not
// the whole window here: game::MinimapRenderer sets the viewport to the square of the
// minimap, and the triangle of composite.vert covers exactly that square.
in vec2 vUv;

// The picture of the minimap (the number of a texture unit): the colour texture of the
// framebuffer post/minimap.frag has drawn into.
uniform sampler2D uMap;

// How much the map hides of the scene behind it: 1 hides it completely.
uniform float uOpacity;

// Output: the color written to the window (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // The colours of the picture are sRGB values already (see post/minimap.frag), and
    // the window takes sRGB values: GL_FRAMEBUFFER_SRGB is off, so OpenGL does not
    // encode what is written here. They are copied without any conversion. Encoding
    // them with linearToSrgb, as composite.frag does with the scene, would encode them
    // a second time and make the map too bright.
    //
    // The alpha is the opacity. The blending of OpenGL then mixes this colour with the
    // scene that is already in the window: colour * alpha + scene * (1 - alpha). Both
    // are sRGB values at that point, so the mix is not the physically right one of
    // linear light. The debug UI and the HUD are mixed in the same way.
    fragColor = vec4(texture(uMap, vUv).rgb, uOpacity);
}
