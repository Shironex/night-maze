#version 410 core
// Fragment shader of the sky: the colour of a fragment is the cube map read in the
// direction the fragment is seen in.
// See docs/modules/renderer/skybox.md

// Input from the vertex shader: the direction of this fragment in world space, already
// interpolated. Its length is not 1, and for reading a cube map it does not have to be.
in vec3 vDirection;

// The six pictures of the sky. A samplerCube, like a sampler2D, holds the NUMBER OF
// A TEXTURE UNIT, set from C++ with gfx::Shader::setInt (glUniform1i). The cube map
// bound to that unit (gfx::Cubemap::bind) is the one that is read.
uniform samplerCube uSkybox;

// The colour of the sky is multiplied by this number: 1 shows the pictures as they are.
uniform float uBrightness;

// What to show. The numbers are the values of game::ViewMode in C++.
//   0: the sky
//   1 and 2 (the debug views of the normals and of the texture coordinates): the
//      direction the cube map is read with, as a colour
uniform int uViewMode;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    if (uViewMode != 0) {
        // The sky has no normal and no (u, v): its texture coordinate is the direction.
        // It is shown with the colour coding of the normals in textured.frag: each
        // component goes from -1..1 to 0..1. The sky towards +X comes out reddish, +Y
        // (up) greenish, +Z bluish, and the opposite directions dark in that channel.
        fragColor = vec4(normalize(vDirection) * 0.5 + 0.5, 1.0);
    } else {
        // texture() with a samplerCube takes a direction (a vec3) instead of (u, v). The
        // graphics card picks the face the direction points at (the axis with the
        // largest component) and the texel on that face.
        //
        // No lighting: the sky gives off its own light. Like the other textures, the
        // pictures are used as they are in the file (no gamma correction before M7).
        vec3 sky = texture(uSkybox, vDirection).rgb;
        fragColor = vec4(sky * uBrightness, 1.0);
    }
}
