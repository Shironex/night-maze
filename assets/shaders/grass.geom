#version 410 core
// Geometry shader of the grass: turns every point into a tuft of a few thin blades.
//
// A geometry shader runs once for every primitive that leaves the vertex shader and
// writes new primitives in its place. Here one point comes in and up to BLADES_PER_TUFT
// triangle strips go out. The vertex buffer therefore holds only one point per tuft, and
// the blades exist on the graphics card alone, made again in every frame: that is why
// they can sway in the wind without any data being sent again.
//
// The game has a second way to make the same blades, without a geometry shader:
// grass_instanced.vert, the default (game::GrassPath). This one is kept as the example of
// a geometry shader. Both take the shape of a blade, the wind and the distance LOD from
// one file, common/grass_blade.glsl, so the two draw the same grass.

// The shape of a blade, the wind and the distance LOD, and their uniforms (uTime,
// uBladeHeight, uWindStrength and the uLod ones).
#include "common/grass_blade.glsl"

// Input: points. Every "in" variable below is an array with one element per vertex of
// the primitive, and a point has one vertex, so the element is always [0].
layout(points) in;

// The most vertices one point can turn into: BLADES_PER_TUFT * VERTICES_PER_BLADE. Keep
// it in step with the two constants in common/grass_blade.glsl. It is a #define and not
// a "const int", because GLSL 4.10 wants a plain number inside layout(...): a constant
// expression is allowed there only from GLSL 4.40 on. The preprocessor writes the number
// in place of the name before the compiler reads the line, so the compiler sees a plain
// 15.
#define TUFT_MAX_VERTICES 15

// Output: triangle strips. max_vertices is a promise to the graphics card, which
// reserves room for that many vertices per point: small is fast.
layout(triangle_strip, max_vertices = TUFT_MAX_VERTICES) out;

// From grass.vert: the random number of the tuft, 0 to 1.
in float vRandom[];

// Uniforms: set from C++ (game::GrassRenderer).
uniform mat4 uView;       // world space to view space
uniform mat4 uProjection; // view space to clip space

// Outputs to the fragment shader, blended across each triangle by the rasterizer.
out vec3 gWorldPosition; // position in world space, for the lighting
out vec2 gBladeUv;       // x: 0 at the left edge of the blade and 1 at the right edge,
                         // y: 0 at the root and 1 at the tip

// Writes one corner of a blade. The view and projection matrices are applied here: this
// is the last stage that handles vertices, so it has to write clip space positions.
void emitBladeCorner(GrassBlade blade, vec3 windOffset, int corner, float scale) {
    vec2 bladeUv;
    vec3 worldPosition = grassBladeCorner(blade, windOffset, corner, scale, bladeUv);
    gWorldPosition = worldPosition;
    gBladeUv = bladeUv;
    gl_Position = uProjection * uView * vec4(worldPosition, 1.0);
    EmitVertex();
}

void main() {
    vec3 tuftRoot = gl_in[0].gl_Position.xyz;
    float tuftRandom = vRandom[0];

    // The wind pushes the whole tuft the same way.
    vec3 windOffset = grassWindOffset(tuftRoot, tuftRandom);

    for (int blade = 0; blade < BLADES_PER_TUFT; ++blade) {
        // A blade the distance LOD has taken away entirely is not written at all: here
        // a geometry shader saves real work, because no vertex of it reaches the card.
        float scale = grassBladeScale(tuftRoot, tuftRandom, blade);
        if (scale <= 0.0) {
            continue;
        }
        GrassBlade shape = grassBlade(tuftRoot, tuftRandom, blade);

        // A triangle strip: left, right, left, right, tip gives three triangles.
        emitBladeCorner(shape, windOffset, ROOT_LEFT, scale);
        emitBladeCorner(shape, windOffset, ROOT_RIGHT, scale);
        emitBladeCorner(shape, windOffset, MIDDLE_LEFT, scale);
        emitBladeCorner(shape, windOffset, MIDDLE_RIGHT, scale);
        emitBladeCorner(shape, windOffset, TIP, scale);
        // The strip of this blade ends here. The next blade starts a new one.
        EndPrimitive();
    }
}
