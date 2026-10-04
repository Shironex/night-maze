#version 410 core
// Vertex shader: runs once for every vertex and decides where it lands on the screen.
// See docs/modules/gfx/shaders.md

// Inputs: the attributes of one vertex, read from the vertex buffer. The location numbers
// are the attribute indices that the C++ code uses when it describes the vertex layout.
layout(location = 0) in vec3 aPosition; // x, y, z
layout(location = 1) in vec3 aColor;    // red, green, blue, each from 0 to 1

// Output to the fragment shader. The rasterizer blends it between the three vertices of
// a triangle, so every fragment receives its own in-between color.
out vec3 vColor;

void main() {
    // gl_Position is the built-in output every vertex shader must write: the position in
    // clip space. There are no matrices yet, so the position from the buffer is used as
    // it is. With w = 1 it is already in normalized device coordinates: x and y from -1
    // to 1 cover the whole window.
    gl_Position = vec4(aPosition, 1.0);

    // Pass the color through unchanged.
    vColor = aColor;
}
