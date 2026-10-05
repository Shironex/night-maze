#version 410 core
// Vertex shader of textured models: places the vertex on the screen and passes its
// texture coordinate and its normal on to the fragment shader.
// See docs/modules/gfx/textures.md

// Inputs: the three attributes of gfx::Vertex. The location numbers are the constants
// POSITION_ATTRIBUTE, NORMAL_ATTRIBUTE and UV_ATTRIBUTE of src/gfx/Vertex.hpp.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the model
layout(location = 1) in vec3 aNormal;   // direction the surface faces, length 1
layout(location = 2) in vec2 aUv;       // texture coordinate (u, v), v = 0 is the bottom

// Uniforms: set from C++ (gfx::Shader::setMat4). uModel changes with every object,
// uView and uProjection are the same for the whole frame.
uniform mat4 uModel;      // local space to world space: where the object stands
uniform mat4 uView;       // world space to view space: where the camera is and looks
uniform mat4 uProjection; // view space to clip space: perspective

// Outputs to the fragment shader. The rasterizer blends them between the three vertices
// of a triangle. The fragment shader declares inputs with the same names and types.
out vec2 vUv;     // texture coordinate
out vec3 vNormal; // normal in world space

void main() {
    // The same chain as in basic.vert, read from right to left: local space, world
    // space, view space, clip space.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);

    // The texture coordinate goes through unchanged: it belongs to the surface, not to
    // the place where the object stands.
    vUv = aUv;

    // A normal is a direction, not a point, so it must turn with the object but must not
    // be moved by the translation. mat3(uModel) is the upper left 3 x 3 part of the
    // matrix: rotation and scale without the translation. That is correct as long as
    // the scale is the same on all three axes, which holds for every object of the
    // maze (scale 1). Unequal scale would need the inverse transpose of that matrix.
    vNormal = mat3(uModel) * aNormal;
}
