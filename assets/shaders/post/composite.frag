#version 410 core
// Fragment shader of the composite pass, the last pass of a frame: reads the picture of
// the scene from the HDR texture and writes it to the window as colours a screen can
// show.
// See docs/modules/renderer/post-process.md

// linearToSrgb. The path is relative to this file.
#include "../common/color.glsl"

// Input from composite.vert: the texture coordinate of this pixel of the screen.
in vec2 vUv;

// The picture of the scene: linear colours, not limited to 1 (GL_RGBA16F). A sampler
// holds the number of a texture unit, set from C++ with glUniform1i.
uniform sampler2D uScene;

// The bloom: the bright parts of the scene, blurred (linear HDR colours, half the size
// of the scene). uBloomEnabled is 1 when it is added to the scene and 0 when the frame
// is drawn without it: the texture is not read then. The glow is multiplied by
// uBloomIntensity first.
uniform sampler2D uBloom;
uniform int uBloomEnabled;
uniform float uBloomIntensity;

// The colours are multiplied by this number, like a longer or a shorter exposure of
// a camera: 1 changes nothing, 2 doubles the light.
uniform float uExposure;

// How the range of the scene is brought into 0..1. The numbers are the values of
// game::ToneMapping in C++.
//   0: none, values above 1 are cut off
//   1: Reinhard
//   2: ACES (a fitted curve)
uniform int uToneMapping;

// Output: the color written to the window (red, green, blue, alpha).
out vec4 fragColor;

// Reinhard: x / (1 + x), per channel. 0 stays 0, 1 becomes one half, and however bright
// the input is, the result stays below 1: nothing is cut off, bright areas keep detail.
// The price is that the whole picture gets darker and flatter.
vec3 toneMapReinhard(vec3 color) {
    return color / (vec3(1.0) + color);
}

// ACES: the look of the film industry reference curve, as the short formula Krzysztof
// Narkowicz fitted to it (2015). A quotient of two quadratic polynomials shaped like an
// S: dark tones are pressed down a little (more contrast), the middle is almost
// straight, and bright values bend softly towards 1. The five numbers are the fit.
vec3 toneMapAces(vec3 color) {
    const float A = 2.51;
    const float B = 0.03;
    const float C = 2.43;
    const float D = 0.59;
    const float E = 0.14;
    return clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);
}

void main() {
    // The steps run in a fixed order, and each one works on the result of the one
    // before. Later effects have their place in it:
    //
    //   1. the scene, in linear HDR colours
    //      (effects that work on light itself, like fog, are added here, before the
    //      exposure: they need linear values that are not cut off)
    //   2. bloom, such an effect: the glow of the bright parts is added
    //   3. exposure
    //   4. tone mapping: from 0..infinity to 0..1
    //      (effects that work on the finished picture, like a vignette, come here)
    //   5. encoding to sRGB, always last
    vec3 color = texture(uScene, vUv).rgb;

    // Bloom: the glow is light, so it is ADDED to the light of the scene, and it is
    // added before the exposure and the tone mapping, which then treat it like the
    // rest of the picture. The bloom texture is half as large as the screen: the linear
    // filter stretches it, and the blur has left nothing sharp in it to look blocky.
    if (uBloomEnabled == 1) {
        color += texture(uBloom, vUv).rgb * uBloomIntensity;
    }

    color *= uExposure;

    if (uToneMapping == 1) {
        color = toneMapReinhard(color);
    } else if (uToneMapping == 2) {
        color = toneMapAces(color);
    } else {
        color = clamp(color, 0.0, 1.0);
    }

    // The screen expects sRGB encoded numbers. This is the one place where the frame
    // is encoded (gamma correction). GL_FRAMEBUFFER_SRGB stays off, so OpenGL does not
    // encode a second time, and the debug UI, drawn after this pass straight into the
    // window, keeps the colours of its theme.
    fragColor = vec4(linearToSrgb(color), 1.0);
}
