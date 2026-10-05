#version 410 core
// Fragment shader of the blur passes of the bloom: one direction of a Gaussian blur.
// Used with post/composite.vert.
// See docs/modules/renderer/post-process.md

// Input from composite.vert: the texture coordinate of this pixel.
in vec2 vUv;

// How many pixels are read on each side of this one. The same number as
// game::BLOOM_BLUR_RADIUS in C++ (src/game/Bloom.hpp): the two must agree.
const int BLUR_RADIUS = 6;

// The picture to blur: linear HDR colours (GL_RGBA16F), read with a linear filter and
// clamped to the edge, so a read past the border repeats the border pixel.
uniform sampler2D uSource;

// The direction of this pass: 1 reads the neighbours to the left and to the right,
// 0 the ones below and above.
uniform int uHorizontal;

// The weights of the kernel, computed in C++ (game::bloomBlurWeights) from the
// Gaussian function: element 0 for this pixel, element d for each of the two pixels
// d pixels away. Together (the centre once, the others twice) they add up to 1.
uniform float uWeights[BLUR_RADIUS + 1];

// Output: the color written to the blur target (red, green, blue, alpha), as a linear
// HDR colour.
out vec4 fragColor;

void main() {
    // A two dimensional Gaussian blur of 13 x 13 pixels would need 169 texture reads.
    // The Gaussian function is SEPARABLE: blurring the rows first and then the columns
    // of the result gives the same picture with 13 + 13 reads. This shader is one of
    // those two passes.

    // The step to the next pixel in texture coordinates: 1 / size. textureSize returns
    // the size of level 0 of the texture in pixels.
    vec2 texel = 1.0 / vec2(textureSize(uSource, 0));
    vec2 texelStep = uHorizontal == 1 ? vec2(texel.x, 0.0) : vec2(0.0, texel.y);

    // The weighted sum: this pixel, then the pairs of neighbours at distance 1, 2, ...
    vec3 sum = texture(uSource, vUv).rgb * uWeights[0];
    for (int pixels = 1; pixels <= BLUR_RADIUS; ++pixels) {
        vec2 offset = texelStep * float(pixels);
        sum += texture(uSource, vUv + offset).rgb * uWeights[pixels];
        sum += texture(uSource, vUv - offset).rgb * uWeights[pixels];
    }
    fragColor = vec4(sum, 1.0);
}
