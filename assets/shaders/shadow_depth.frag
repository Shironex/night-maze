#version 410 core
// Fragment shader of the depth pass of a shadow map. It writes nothing: the framebuffer
// of a shadow map has no colour texture, and the depth of a fragment is stored by the
// graphics card on its own (gl_FragCoord.z, after the depth test). A program still has
// to have a fragment shader, so this one is empty.

void main() {
}
