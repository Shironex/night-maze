#version 410 core
// Fragment shader of the background of the main menu: one picture (a frame of the menu
// video, or the still picture in its place) over the whole window. Used with
// post/composite.vert.

// Input from composite.vert: the texture coordinate of this pixel, (0, 0) in the bottom
// left corner of the window and (1, 1) in the top right one.
in vec2 vUv;

// The picture (the number of a texture unit). Its first row is the TOP row: a video
// decoder delivers its rows from the top down, and OpenGL puts the first row it is given
// at v = 0.
uniform sampler2D uPicture;

// The part of the picture that is shown, in its texture coordinates (gfx::coverFit):
// the picture covers the window without being stretched, and what sticks out on two
// sides is cut off. Four numbers from 0 to 1: the left and the right edge, the top and
// the bottom edge. For a window with the shape of the picture they are 0, 1, 0, 1.
uniform float uShownLeft;
uniform float uShownRight;
uniform float uShownTop;
uniform float uShownBottom;

// The scrim: the colours are multiplied by this number. 1 leaves the picture as it is,
// a smaller number darkens all of it evenly, so the text of the menu stays readable on
// a bright frame.
uniform float uBrightness;

// Output: the color written to the window (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // From the window to the picture. The window counts v from the bottom up and the
    // picture has its top row at v = 0, so the top of the window (vUv.y = 1) reads the
    // top edge of the shown part and the bottom of the window its bottom edge.
    vec2 uv = vec2(mix(uShownLeft, uShownRight, vUv.x), mix(uShownBottom, uShownTop, vUv.y));

    // The bytes of the picture are sRGB values already, like the window expects them:
    // a video and a picture file are stored encoded for a screen. The texture is
    // a GL_RGBA8 one, so they arrive here unchanged, and GL_FRAMEBUFFER_SRGB is off, so
    // they are written unchanged. Encoding them with linearToSrgb, as composite.frag
    // does with the scene, would encode them a second time and make the picture pale.
    //
    // The scrim multiplies those encoded values. That is not the physically right way to
    // dim light, but it is a look and not a lighting step, and the menu documents on top
    // are blended in the same encoded values.
    vec3 color = texture(uPicture, uv).rgb * uBrightness;

    fragColor = vec4(color, 1.0);
}
