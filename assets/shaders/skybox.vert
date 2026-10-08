#version 410 core
// Vertex shader of the sky: places a cube around the camera so that it turns with the
// camera, never moves with it and lies at the largest possible depth.

// Input: only the position. The mesh also carries a normal (location 1), a texture
// coordinate (location 2) and a tangent (location 3), but a shader may leave attributes
// it does not need unread.
layout(location = 0) in vec3 aPosition; // a corner of the cube, -1 or 1 on every axis

// Uniforms: set from C++ (gfx::Shader::setMat4). The same two matrices the maze is drawn
// with. There is no uModel: the cube is not placed anywhere in the world.
uniform mat4 uView;       // world space to view space: where the camera is and looks
uniform mat4 uProjection; // view space to clip space: perspective

// Output to the fragment shader: the direction from the middle of the cube to this
// vertex. The rasterizer blends it between the vertices, so every fragment gets the
// direction it is seen in. That direction is what a cube map is read with.
out vec3 vDirection;

void main() {
    // The cube is centred on the origin, so the position of a corner is also the
    // direction to it. It is a direction of the WORLD (it is passed on before the view
    // matrix is applied), so the sky stays fixed to the world when the camera turns.
    vDirection = aPosition;

    // A view matrix turns the world and then moves it. mat3(uView) keeps the upper left
    // 3 x 3 part, the turn, and mat4(...) of that puts it back into a 4 x 4 matrix
    // without the move. The camera is then always in the middle of the cube, wherever
    // the player walks: the sky never comes closer, which is how something very far
    // away looks.
    mat4 viewWithoutTranslation = mat4(mat3(uView));
    vec4 position = uProjection * viewWithoutTranslation * vec4(aPosition, 1.0);

    // After this shader the graphics card divides x, y and z by w, and z / w is the
    // depth. Writing w into z makes the depth w / w = 1.0 for every vertex: the far
    // plane, behind everything else. The size of the cube then no longer matters for
    // the depth. The sky is drawn last with the depth test GL_LEQUAL (game::Skybox), so
    // it only fills the pixels nothing else was drawn on.
    gl_Position = position.xyww;
}
