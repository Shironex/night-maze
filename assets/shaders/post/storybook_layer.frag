#version 410 core
// Fragment shader of the storybook intro: one layer of a painted picture over the whole
// window, blended over the layers behind it. Used with post/composite.vert.

// Input from composite.vert: (0, 0) in the bottom left corner of the window and (1, 1)
// in the top right one.
in vec2 vUv;

// The layer (the number of a texture unit). Its first row is the TOP row, like the still
// picture of the menu (post/menu_background.frag).
uniform sampler2D uPicture;

// The part of the picture that is shown (game::storybookLayerUv): the left, right, top
// and bottom edge in its texture coordinates.
uniform float uShownLeft;
uniform float uShownRight;
uniform float uShownTop;
uniform float uShownBottom;

// How much of the layer is there, 0 to 1: 1 except while its picture fades in.
uniform float uOpacity;

out vec4 fragColor;

void main() {
    vec2 uv = vec2(mix(uShownLeft, uShownRight, vUv.x), mix(uShownBottom, uShownTop, vUv.y));
    // The bytes are sRGB already and are written as they are, like the still picture of
    // the menu: no decoding here and no encoding on the way out. The blending is done
    // in those encoded values, as a paint program does it.
    vec4 color = texture(uPicture, uv);
    fragColor = vec4(color.rgb, color.a * uOpacity);
}
