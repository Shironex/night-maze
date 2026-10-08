// Normal mapping shared by lit.frag and textured.frag: the sampler of the normal map, its
// switch and the function that turns the normal of a model into the normal of the relief.
// This file is not a shader of its own. It has no #version line: the shader loader puts
// its text in place of the line  #include "common/normal_map.glsl"  (gfx/ShaderSource.hpp).

// The normal map of the part being drawn. Like uTexture it holds the number of a texture
// unit, a different one: the colour picture is on unit 0, the normal map on unit 1. A part
// without a normal map of its own gets a 1 x 1 "flat" map from C++, so the shader never
// has to ask whether there is one.
uniform sampler2D uNormalMap;
// Whether the normal map is used at all: the checkbox "Normal mapping" of the Assets
// panel. A uniform nobody has set is false, so a freshly loaded program starts without
// normal mapping.
uniform bool uNormalMapEnabled;

// The normal to shade one fragment with, in world space, with length 1.
// normal and tangent are the two inputs from the vertex shader, both in world space.
//
// Without normal mapping it is the normal of the model, the same over a whole flat face.
// With it, the normal comes from the normal map, which makes a flat face look as if it
// had joints and bumps: the shape stays flat, only the light on it changes.
vec3 surfaceNormal(vec3 normal, vec3 tangent, vec2 uv) {
    // Blending between the vertices shortens a normal, so it is brought back to length 1.
    vec3 n = normalize(normal);
    if (!uNormalMapEnabled) {
        return n;
    }

    // A normal map is written in tangent space, the space of the picture itself:
    //   x: to the right in the picture (u grows), the tangent T
    //   y: up in the picture (v grows), the bitangent B
    //   z: out of the surface, the normal N
    // T comes from the model and is perpendicular to N (assets::computeTangents does
    // that once, when the model is loaded). B is perpendicular to both. cross(N, T) and
    // not cross(T, N): with N towards the viewer and T to the right, it points up. The
    // other order would point down and turn every joint into a ridge.
    vec3 t = normalize(tangent);
    vec3 b = cross(n, t);

    // The three vectors as the columns of a matrix. Multiplying by it turns a direction
    // from tangent space into world space: x * T + y * B + z * N.
    mat3 tangentToWorld = mat3(t, b, n);

    // The texture stores each component as a colour from 0 to 1. Times two, minus one
    // brings it back to -1..1. A flat texel (128, 128, 255) becomes about (0, 0, 1): the
    // normal of the model, unchanged.
    vec3 mapped = texture(uNormalMap, uv).rgb * 2.0 - 1.0;

    // normalize: the filter of the texture blends neighbouring texels, and the mipmaps
    // blend whole areas, and a blend of unit vectors is shorter than 1.
    return normalize(tangentToWorld * mapped);
}
