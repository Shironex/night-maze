#version 410 core
// Fragment shader of the bright pass, the first pass of the bloom: keeps the light of
// the scene that is brighter than a threshold and writes black everywhere else.
// Used with post/composite.vert.

// luminance. The path is relative to this file.
#include "../common/color.glsl"

// Input from composite.vert: the texture coordinate of this pixel.
in vec2 vUv;

// The picture of the scene: linear HDR colours (GL_RGBA16F). The target of this pass
// is half as large in each direction, so one pixel here lies between four pixels of the
// scene, and the linear filter of the texture returns their average (exactly so when
// the width and the height of the scene are even numbers).
uniform sampler2D uScene;

// Brightness (luminance) above which light takes part in the bloom.
uniform float uThreshold;

// Output: the color written to the bright pass texture (red, green, blue, alpha), as
// a linear HDR colour.
out vec4 fragColor;

// A brightness below this counts as black. It keeps the division below away from 0 / 0.
const float MIN_LUMINANCE = 0.0001;

void main() {
    vec3 color = texture(uScene, vUv).rgb;
    float brightness = luminance(color);

    // The share of the brightness that lies above the threshold: 0 for a pixel at or
    // under it, close to 1 for a very bright one. The colour is multiplied by that
    // share, so all three channels shrink by the same factor and the hue stays. There
    // is no jump at the threshold: a pixel just above it keeps almost nothing.
    float share = max(brightness - uThreshold, 0.0) / max(brightness, MIN_LUMINANCE);
    fragColor = vec4(color * share, 1.0);
}
