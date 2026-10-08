#version 410 core
// Vertex shader of the grass: it does almost nothing. One vertex is one tuft, a point on
// the ground, and the blades are made of it in the next stage, grass.geom.

// Inputs: two of the attributes of gfx::Vertex. The grass is drawn as GL_POINTS from
// a mesh that holds one vertex per tuft. Normal and tangent are not read.
layout(location = 0) in vec3 aPosition; // the root of the tuft, already in world space
layout(location = 2) in vec2 aUv;       // x: the random number of the tuft, 0 to 1

// Output to the geometry shader. Between a vertex and a geometry shader nothing is
// blended: the geometry shader gets the value of every vertex of its primitive, for
// a point exactly one.
out float vRandom; // the random number of the tuft

void main() {
    // The position stays in world space: no view and no projection matrix here. The
    // geometry shader builds the blades in world space, where "up" and the direction of
    // the wind are simple, and applies both matrices to the vertices it makes.
    gl_Position = vec4(aPosition, 1.0);
    vRandom = aUv.x;
}
