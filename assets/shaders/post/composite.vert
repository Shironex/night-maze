#version 410 core
// Vertex shader of the post-processing passes: one triangle that covers the whole
// screen, made without any vertex data.

// There is no input. The draw call is glDrawArrays(GL_TRIANGLES, 0, 3) with a vertex
// array object that has no attributes (a Core profile still needs one bound), and the
// three corners are computed from gl_VertexID, the number of the vertex: 0, 1, 2.

// Output to the fragment shader: the texture coordinate of the screen, (0, 0) in the
// bottom left corner and (1, 1) in the top right one.
out vec2 vUv;

void main() {
    // Vertex 0 -> (0, 0), vertex 1 -> (2, 0), vertex 2 -> (0, 2). The first line takes
    // bit 0 of the number for x, the second bit 1 for y.
    float x = float(gl_VertexID % 2) * 2.0;
    float y = float(gl_VertexID / 2) * 2.0;
    vUv = vec2(x, y);

    // From 0..2 to -1..3 in clip space: the corners (-1, -1), (3, -1) and (-1, 3). The
    // screen is the square from -1 to 1, and this one triangle covers it completely.
    // What sticks out is clipped away. One triangle instead of two that share
    // a diagonal: no pixel is shaded twice along a seam. Depth 0 and w 1: the pass is
    // drawn with the depth test off.
    gl_Position = vec4(vUv * 2.0 - 1.0, 0.0, 1.0);
}
