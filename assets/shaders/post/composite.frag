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

// The fog. uFogEnabled is 1 when it is mixed into the scene and 0 when the frame is
// drawn without it: the depth texture is not read then.
uniform int uFogEnabled;
// The depth of the scene: for every pixel one number in the red channel, from 0 (near
// plane) to 1 (far plane and the sky).
uniform sampler2D uDepth;
// The inverse of projection * view of the scene: from the screen back to the world.
uniform mat4 uInverseViewProjection;
// Where the scene is seen from, in world space.
uniform vec3 uEye;
// How thick the fog is at and below uFogBaseHeight, per metre.
uniform float uFogDensity;
// The world height (y) up to which the fog has its full density, in metres.
uniform float uFogBaseHeight;
// How fast the fog thins out above that height, per metre.
uniform float uFogHeightFalloff;
// The colour of the fog, a linear colour (C++ has converted it from sRGB).
uniform vec3 uFogColor;

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

// The vignette. uVignetteEnabled is 1 when the corners are darkened and 0 when not.
// uVignetteStrength is the share of the light the corners lose (0 to 1), and
// uVignetteRadius the distance from the middle of the screen at which the darkening
// starts, in texture coordinates.
uniform int uVignetteEnabled;
uniform float uVignetteStrength;
uniform float uVignetteRadius;

// Output: the color written to the window (red, green, blue, alpha).
out vec4 fragColor;

// The middle of the screen as a texture coordinate, and the distance from it to
// a corner: the length of (0.5, 0.5), the square root of 0.5. The same numbers as
// SCREEN_CENTER and VIGNETTE_CORNER_DISTANCE in src/game/Vignette.hpp.
const vec2 SCREEN_CENTER = vec2(0.5, 0.5);
const float VIGNETTE_CORNER_DISTANCE = 0.70710678;

// The world position of the surface a pixel shows, from its texture coordinate on the
// screen and the value of the depth texture there. The same steps as
// game::worldPositionFromDepth in src/game/Fog.cpp.
vec3 worldPositionFromDepth(vec2 uv, float depth) {
    // Step 1: normalised device coordinates. The texture coordinate and the depth run
    // from 0 to 1, and in normalised device coordinates all three axes run from -1 to
    // 1, so each value is doubled and moved down by 1. The fourth component 1 makes it
    // a point.
    vec4 ndc = vec4(vec3(uv, depth) * 2.0 - 1.0, 1.0);

    // Step 2: the inverse of projection * view takes the point back towards the world.
    vec4 world = uInverseViewProjection * ndc;

    // Step 3: divide by w. On the way to the screen the graphics card divided the clip
    // space position by its w (the perspective division). A matrix cannot undo
    // a division, so the result of step 2 is the world position divided by that w, and
    // its own fourth component is 1 divided by that w. Dividing by it gives the world
    // position back.
    return world.xyz / world.w;
}

// How much of the full density the fog has at a world height: 1 at and below the base
// height, and above it falling towards 0, by the same share for every metre. The same
// formula as game::fogHeightFactor in src/game/Fog.cpp.
float fogHeightFactor(float height) {
    float heightAboveBase = max(height - uFogBaseHeight, 0.0);
    return exp(-uFogHeightFalloff * heightAboveBase);
}

// How much of a surface is replaced by the colour of the fog, 0 to 1. Exponential fog:
// every metre of fog lets the same share of the light through, so after d metres
// exp(-density * d) of it is left, and the fog takes the rest. The same formula as
// game::fogAmount in src/game/Fog.cpp. (The parameter is not called distance: that is
// the name of a built-in function of GLSL.)
float fogAmount(float heightFactor, float distanceToEye) {
    return 1.0 - exp(-uFogDensity * heightFactor * distanceToEye);
}

// The number a pixel is multiplied by for the vignette: 1 inside the radius, then
// falling smoothly to 1 - uVignetteStrength in the corners. The distance is measured in
// texture coordinates, which run from 0 to 1 in both directions whatever the shape of
// the window, so the bright middle is an ellipse of the shape of the window and the four
// corners are always equally dark. The same formula as game::vignetteFactor in
// src/game/Vignette.cpp.
float vignetteFactor(vec2 uv) {
    float distanceToCenter = length(uv - SCREEN_CENTER);
    // smoothstep is 0 up to the first edge, 1 from the second edge on and an S shaped
    // curve in between.
    float darkening = smoothstep(uVignetteRadius, VIGNETTE_CORNER_DISTANCE, distanceToCenter);
    return 1.0 - uVignetteStrength * darkening;
}

// Reinhard: x / (1 + x), per channel. 0 stays 0, 1 becomes one half, and however bright
// the input is, the result stays below 1: nothing is cut off, bright areas keep detail.
// The price is that the whole picture gets darker and flatter.
vec3 toneMapReinhard(vec3 color) {
    return color / (vec3(1.0) + color);
}

// ACES: the look of the film industry reference curve, as the short formula Krzysztof
// Narkowicz fitted to it (2016). A quotient of two quadratic polynomials shaped like an
// S: dark tones are pressed down hard (0.01 comes out as about 0.0038, which gives more
// contrast), the middle is almost straight, and bright values bend softly towards 1.
// The five numbers are the fit.
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
    // before:
    //
    //   1. the scene, in linear HDR colours
    //   2. fog: an effect that works on light itself, so it comes before the exposure,
    //      on linear values that are not cut off
    //   3. bloom, such an effect too: the glow of the bright parts is added
    //   4. exposure
    //   5. tone mapping: from 0..infinity to 0..1
    //   6. vignette: an effect that works on the finished picture
    //   7. encoding to sRGB, always last
    vec3 color = texture(uScene, vUv).rgb;

    // Fog: the further away a surface is and the lower it lies, the more of its colour
    // is replaced by the colour of the fog. The distance is the length of the straight
    // line from the eye to the surface, so the fog on a wall does not change when the
    // camera turns. The height is taken at the surface only, not along that whole
    // line: see game::fogAmountAt for what that gets wrong.
    //
    // The sky needs no case of its own. Its pixels have depth 1, which gives a point on
    // the far clipping plane in the direction of the pixel. That point is far away, so
    // the distance alone would hide the sky. But looking upwards it is also very high,
    // where the height factor is almost 0: the moon and the stars stay clear, and only
    // the sky close to the horizon, and below it, turns into fog. The far plane is flat
    // and turns with the camera, so on that low strip of sky (and only there) the
    // amount also depends on where on the screen a pixel is.
    if (uFogEnabled == 1) {
        float depth = texture(uDepth, vUv).r;
        vec3 position = worldPositionFromDepth(vUv, depth);
        float amount = fogAmount(fogHeightFactor(position.y), length(position - uEye));
        color = mix(color, uFogColor, amount);
    }

    // Bloom: the glow is light, so it is ADDED to the light of the scene, and it is
    // added before the exposure and the tone mapping, which then treat it like the
    // rest of the picture. It comes AFTER the fog: the glow of a crystal shines through
    // the fog instead of being replaced by its colour. The bloom texture is half as
    // large as the screen: the linear filter stretches it, and the blur has left
    // nothing sharp in it to look blocky.
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

    // Vignette: the corners of the finished picture are darkened. It comes after the
    // tone mapping, so that it darkens what is seen and does not just shift the input
    // of the curve, and before the encoding, because multiplying light by a number is
    // only right on linear values.
    if (uVignetteEnabled == 1) {
        color *= vignetteFactor(vUv);
    }

    // The screen expects sRGB encoded numbers. This is the one place where the frame
    // is encoded (gamma correction). GL_FRAMEBUFFER_SRGB stays off, so OpenGL does not
    // encode a second time, and the debug UI, drawn after this pass straight into the
    // window, keeps the colours of its theme.
    fragColor = vec4(linearToSrgb(color), 1.0);
}
