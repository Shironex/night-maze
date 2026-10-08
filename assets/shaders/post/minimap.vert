#version 410 core
// Vertex shader of the minimap: places the flat shapes of the map (floors, walls, gate,
// crystals, player) in the picture of the map.

// Input: one corner of a triangle (game::MinimapVertex in C++). The numbers after
// "location" are the attribute numbers game::MinimapRenderer describes the buffer with.
layout(location = 0) in vec2 aPosition; // a place in the maze seen from above, in metres:
                                        // x is the world x (east), y is the world z (south)
layout(location = 1) in vec3 aColor;    // the colour of the shape, an sRGB value

// Set from C++ (game::minimapProjection): the orthographic projection that takes the
// square piece of the world around the maze to clip space, with north at the top.
uniform mat4 uMapToClip;

// Output to the fragment shader. All three corners of a triangle carry the same colour,
// so every pixel of the triangle gets that colour.
out vec3 vColor;

void main() {
    // The map is flat: depth 0 and w 1. There is no model and no view matrix, the
    // positions are already places in the world.
    gl_Position = uMapToClip * vec4(aPosition, 0.0, 1.0);
    vColor = aColor;
}
