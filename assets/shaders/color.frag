#version 410 core
// Fragment shader for shapes drawn in one flat colour: the lines of the collision boxes
// and spheres.
// See docs/modules/scene/collision.md

// The colour of the whole shape (red, green, blue), set from C++ (gfx::Shader::setVec3).
uniform vec3 uColor;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // Every fragment gets the same colour. Alpha 1 means fully opaque.
    fragColor = vec4(uColor, 1.0);
}
