#version 410 core
// Fragment shader of textured models: the colour of a fragment is the texture at its
// texture coordinate, multiplied by a tint. Two debug views show the normal or the
// texture coordinate as a colour instead. There is no lighting here.
// See docs/modules/gfx/textures.md

// Inputs from the vertex shader: same names and types as its outputs, already
// interpolated for this fragment.
in vec2 vUv;     // texture coordinate
in vec3 vNormal; // normal in world space, no longer exactly of length 1

// The texture to read. A sampler does not hold a texture: it holds the NUMBER OF A
// TEXTURE UNIT, set from C++ with gfx::Shader::setInt. The texture bound to that unit
// (gfx::Texture2D::bind) is the one that is read.
uniform sampler2D uTexture;

// Colour the texture is multiplied by: the diffuse colour of the material. White
// (1, 1, 1) leaves the texture unchanged.
uniform vec3 uTint;

// What to show. The numbers are the values of game::ViewMode in C++.
//   0: the texture multiplied by the tint (the normal picture)
//   1: the normal as a colour
//   2: the texture coordinate as a colour
uniform int uViewMode;

// Output: the color written to the framebuffer (red, green, blue, alpha).
out vec4 fragColor;

void main() {
    if (uViewMode == 1) {
        // Interpolation between vertices can shorten a normal, so its length is brought
        // back to 1. Each component is then between -1 and 1, and a colour needs 0 to 1:
        // half of it plus one half. A surface facing +X comes out reddish, +Y (up)
        // greenish, +Z bluish, and the opposite directions dark in that channel.
        vec3 normal = normalize(vNormal);
        fragColor = vec4(normal * 0.5 + 0.5, 1.0);
    } else if (uViewMode == 2) {
        // u goes to red and v to green. The coordinates of the models run past 1 (the
        // texture repeats), so only the fractional part is shown: the colour starts
        // again from black wherever the texture starts again.
        fragColor = vec4(fract(vUv), 0.0, 1.0);
    } else {
        // texture() reads the texture at vUv with the filter, the mipmaps and the
        // wrapping set in OpenGL. It returns red, green, blue, alpha. Only the colour is
        // used: the models are opaque, so alpha is written as 1.
        vec3 texel = texture(uTexture, vUv).rgb;
        fragColor = vec4(texel * uTint, 1.0);
    }
}
