#version 410 core
// Vertex shader for shapes drawn in one flat colour: the lines of the collision boxes
// and the markers of the point lights.
// See docs/modules/scene/collision.md

// Input: only the position. The mesh also carries a normal (location 1), a texture
// coordinate (location 2) and a tangent (location 3), but a shader may leave attributes
// it does not need unread.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the shape

// Uniforms: set from C++ (gfx::Shader::setMat4).
uniform mat4 uModel;      // local space to world space
uniform mat4 uView;       // world space to view space
uniform mat4 uProjection; // view space to clip space

void main() {
    // The same chain as in basic.vert: local, world, view, clip space.
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
