#version 410 core
// Vertex shader of the depth pass of a shadow map: places the vertex as the LIGHT sees it.
// Nothing else is needed, because the pass draws no colours. The depth buffer of the
// shadow map is filled by the depth test, as in every other pass.
// See docs/modules/renderer/shadows.md

// Input: one of the four attributes of gfx::Vertex. The normal, the texture coordinate
// and the tangent are not read: a depth has no colour and no lighting.
layout(location = 0) in vec3 aPosition; // x, y, z in the local space of the model

// Uniforms: set from C++. The same three names as in lit.vert, so the classes that draw
// the maze work with this program as they are. uView and uProjection are not the ones
// of the camera here: they are the view and the projection of the light
// (scene::LightSpace).
uniform mat4 uModel;      // local space to world space
uniform mat4 uView;       // world space to the space of the light
uniform mat4 uProjection; // the space of the light to its clip space

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
