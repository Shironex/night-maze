#version 410 core
// Vertex shader of the instanced grass: makes the blades without a geometry shader.
//
// Instancing draws one mesh many times with one draw call (glDrawElementsInstanced).
// Every copy is an instance, and gl_InstanceID counts them. The mesh here is one tuft:
// the corners of BLADES_PER_TUFT blades, flat and with no place in the world yet. An
// attribute with a divisor of 1 (glVertexAttribDivisor) does not move on with every
// vertex but with every instance: all corners of one copy read the same tuft. So the card
// reads the mesh once for every tuft, and every copy puts the same blades at the root of
// its own tuft, turned, sized and swayed by the random number of that tuft.
//
// The geometry shader grass.geom draws the same blades from one point per tuft. Both take
// the shape of a blade, the wind and the distance LOD from common/grass_blade.glsl, so
// the two paths cannot drift apart. This one is the default (game::GrassPath): a Mac has
// no geometry stage in its graphics card, and its driver imitates it slowly, while
// instancing is a plain feature of every card.

// The shape of a blade, the wind and the distance LOD, and their uniforms (uTime,
// uBladeHeight, uWindStrength and the uLod ones).
#include "common/grass_blade.glsl"

// Per vertex, from the mesh of one tuft: which blade of the tuft this corner belongs to
// (0 to BLADES_PER_TUFT - 1) and which corner of the blade it is (ROOT_LEFT to TIP). They
// are whole numbers stored as floats: an attribute of whole numbers would need
// glVertexAttribIPointer, for two small numbers that a float holds exactly.
layout(location = 0) in vec2 aBladeCorner;

// Per instance (divisor 1), from the buffer of tufts: the root of the tuft in world space
// and its random number, 0 to 1. grass.vert reads the same buffer, one point per tuft.
layout(location = 1) in vec3 aTuftRoot;
layout(location = 2) in float aTuftRandom;

uniform mat4 uView;       // world space to view space
uniform mat4 uProjection; // view space to clip space

// Outputs to grass.frag, the fragment shader of both paths. The names are the ones
// grass.geom writes ("g" for geometry), so the one fragment shader serves both programs.
out vec3 gWorldPosition; // position in world space, for the lighting
out vec2 gBladeUv;       // x: across the blade, y: 0 at the root and 1 at the tip

void main() {
    int blade = int(aBladeCorner.x);
    int corner = int(aBladeCorner.y);

    // Exactly what grass.geom does for this corner. The vertex shader runs once per
    // corner, so the blade and the wind are worked out five times per blade, where the
    // geometry shader works them out once: cheap sums next to what a geometry shader
    // costs on a card that has to imitate it.
    vec3 windOffset = grassWindOffset(aTuftRoot, aTuftRandom);
    float scale = grassBladeScale(aTuftRoot, aTuftRandom, blade);
    GrassBlade shape = grassBlade(aTuftRoot, aTuftRandom, blade);

    vec2 bladeUv;
    vec3 worldPosition = grassBladeCorner(shape, windOffset, corner, scale, bladeUv);
    gWorldPosition = worldPosition;
    gBladeUv = bladeUv;
    gl_Position = uProjection * uView * vec4(worldPosition, 1.0);
}
