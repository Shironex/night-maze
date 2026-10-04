#version 410 core
// Fragment shader: runs once for every fragment (pixel candidate) and decides its color.
// See docs/modules/gfx/shaders.md

// Input from the vertex shader: same name and type as its "out vec3 vColor". The value is
// already interpolated for this fragment.
in vec3 vColor;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    // Alpha 1 means fully opaque.
    fragColor = vec4(vColor, 1.0);
}
