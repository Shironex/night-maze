#version 410 core
// Fragment shader of the attachment previews: turns the colour or the depth texture of
// a framebuffer (the scene, a shadow map) into a small picture the debug UI can show.
// Used with post/composite.vert.

// linearToSrgb and linearDepth. The paths are relative to this file.
#include "../common/color.glsl"
#include "../common/depth.glsl"

// Input from composite.vert: the texture coordinate of this pixel.
in vec2 vUv;

// The attachment to show (the number of a texture unit): the colour texture in mode 0,
// a depth texture in modes 1 and 2.
uniform sampler2D uSource;

// What uSource is. The numbers are the values of game::AttachmentPreview in C++.
//   0: HDR colour
//   1: depth of a perspective view (the scene, the shadow map of the flashlight), shown
//      as a distance
//   2: depth as it is stored (the shadow map of the moon)
uniform int uMode;

// For the depth of mode 1: the clipping planes of the view it was drawn with (the
// camera for the scene, the flashlight for its shadow map) and the distance in metres
// that is shown as white.
uniform float uNear;
uniform float uFar;
uniform float uDepthRange;

// Output: the color written to the preview texture (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    if (uMode == 1) {
        // The stored depth as it is would be an almost white picture: everything
        // farther than a few metres is above 0.95 (see linearDepth). So it is turned
        // back into metres and shown from black (at the camera) to white (uDepthRange
        // metres away or more). The sky, at the far plane, is white. The grey is
        // written as it is: it is a measure, not light, so it is not encoded. The
        // shadow map of the flashlight is shown the same way, with the flashlight in
        // the place of the camera and its reach as the distance that is white.
        float metres = linearDepth(texture(uSource, vUv).r, uNear, uFar);
        fragColor = vec4(vec3(clamp(metres / uDepthRange, 0.0, 1.0)), 1.0);
    } else if (uMode == 2) {
        // The shadow map of a directional light is drawn with an orthographic
        // projection, whose stored depth grows evenly with the distance. It needs no
        // conversion: black is the near plane of the light, white its far plane (and
        // every texel nothing was drawn into).
        fragColor = vec4(vec3(texture(uSource, vUv).r), 1.0);
    } else {
        // The colour attachment holds linear values that may be above 1. The debug UI
        // draws a texture without any conversion, so the picture is encoded here.
        // Without exposure and tone mapping: this is the content of the buffer, with
        // everything above 1 cut off, not the finished frame.
        fragColor = vec4(linearToSrgb(texture(uSource, vUv).rgb), 1.0);
    }
}
