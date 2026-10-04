#version 410 core
// Vertex shader: runs once for every vertex and decides where it lands on the screen.
// See docs/modules/gfx/shaders.md

// Inputs: the attributes of one vertex, read from the vertex buffer. The location numbers
// are the attribute indices that the C++ code uses when it describes the vertex layout.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the object
layout(location = 1) in vec3 aColor;    // red, green, blue, each from 0 to 1

// Uniforms: set from C++ (gfx::Shader::setMat4), the same for every vertex of one draw call.
// See docs/modules/scene/transforms-camera.md
uniform mat4 uModel;      // local space to world space: where the object stands
uniform mat4 uView;       // world space to view space: where the camera is and looks
uniform mat4 uProjection; // view space to clip space: perspective

// Output to the fragment shader. The rasterizer blends it between the three vertices of
// a triangle, so every fragment receives its own in-between color.
out vec3 vColor;

void main() {
    // gl_Position is the built-in output every vertex shader must write: the position in
    // clip space. The expression is read from right to left, the matrix nearest to the
    // vector is applied first:
    //   vec4(aPosition, 1.0)  the vertex in local space, w = 1 because it is a point
    //   uModel * ...          the vertex in world space
    //   uView * ...           the vertex in view space, as seen from the camera
    //   uProjection * ...     the vertex in clip space
    // After this shader the graphics card divides x, y and z by w (the distance from the
    // camera), which is what makes distant things small.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    // Pass the color through unchanged.
    vColor = aColor;
}
