#version 410 core
// Geometry shader of the grass: turns every point into a tuft of a few thin blades.
// See docs/modules/renderer/grass-geometry.md
//
// A geometry shader runs once for every primitive that leaves the vertex shader and
// writes new primitives in its place. Here one point comes in and BLADE_COUNT triangle
// strips go out. The vertex buffer therefore holds only one point per tuft, and the
// blades exist on the graphics card alone, made again in every frame: that is why they
// can sway in the wind without any data being sent again.

// Input: points. Every "in" variable below is an array with one element per vertex of
// the primitive, and a point has one vertex, so the element is always [0].
layout(points) in;

// A tuft has BLADE_COUNT blades, and a blade is a triangle strip of VERTICES_PER_BLADE
// vertices: two at the root, two half way up and one at the tip, which gives three
// triangles that get narrower towards the tip.
const int BLADE_COUNT = 3;
const int VERTICES_PER_BLADE = 5;

// The most vertices one point can turn into: BLADE_COUNT * VERTICES_PER_BLADE. Keep it
// in step with the two constants above. It is a #define and not a "const int", because
// GLSL 4.10 wants a plain number inside layout(...): a constant expression is allowed
// there only from GLSL 4.40 on. The preprocessor writes the number in place of the name
// before the compiler reads the line, so the compiler sees a plain 15.
#define TUFT_MAX_VERTICES 15

// Output: triangle strips. max_vertices is a promise to the graphics card, which
// reserves room for that many vertices per point: small is fast.
layout(triangle_strip, max_vertices = TUFT_MAX_VERTICES) out;

// From grass.vert: the random number of the tuft, 0 to 1.
in float vRandom[];

// Uniforms: set from C++ (game::GrassRenderer).
uniform mat4 uView;          // world space to view space
uniform mat4 uProjection;    // view space to clip space
uniform float uTime;         // seconds, always growing: the clock of the wind
uniform float uBladeHeight;  // height of the tallest blades in metres
uniform float uWindStrength; // 0: still air, 1: the default breeze

// Outputs to the fragment shader, blended across each triangle by the rasterizer.
out vec3 gWorldPosition; // position in world space, for the lighting
out vec2 gBladeUv;       // x: 0 at the left edge of the blade and 1 at the right edge,
                         // y: 0 at the root and 1 at the tip

const float TWO_PI = 6.2831853;
const vec3 UP = vec3(0.0, 1.0, 0.0);

// Half of the width of a blade at its root, in metres. The width shrinks to 0 at the tip.
const float ROOT_HALF_WIDTH = 0.02;
// The blades of a tuft do not grow from one point: each root is moved this far away from
// the middle of the tuft, in the direction the blade leans.
const float ROOT_SPREAD = 0.025;
// How far the tip leans away from the middle of the tuft, as a part of the blade height.
const float LEAN = 0.4;
// The shortest blades are this part of uBladeHeight.
const float SHORTEST_BLADE = 0.6;
// The level, from 0 at the root to 1 at the tip, at which a blade has its two middle
// vertices.
const float HALF_LEVEL = 0.5;
// The two numbers that make a second random number out of the one of the tuft (see
// main): what it is multiplied by, and what is added for every blade. Nothing depends
// on their exact values. They only must not be whole numbers, or simple fractions of
// each other.
const float BLADE_RANDOM_FACTOR = 7.0;
const float BLADE_RANDOM_STEP = 0.37;

// The wind. It blows along WIND_DIRECTION (x and z of the world, length 1). The tip of
// a full blade swings WIND_REACH metres to each side at wind strength 1, WIND_SPEED
// radians of the swing pass per second, and a tuft one metre further down the wind is
// WIND_PHASE_PER_METRE radians behind: gusts travel over the grass like waves.
const vec2 WIND_DIRECTION = vec2(0.94, 0.34);
const float WIND_REACH = 0.06;
const float WIND_SPEED = 1.9;
const float WIND_PHASE_PER_METRE = 0.9;

// Writes one vertex of a blade. The view and projection matrices are applied here: this
// is the last stage that handles vertices, so it has to write clip space positions.
void emitBladeVertex(vec3 worldPosition, vec2 bladeUv) {
    gWorldPosition = worldPosition;
    gBladeUv = bladeUv;
    gl_Position = uProjection * uView * vec4(worldPosition, 1.0);
    EmitVertex();
}

void main() {
    vec3 tuftRoot = gl_in[0].gl_Position.xyz;
    float tuftRandom = vRandom[0];

    // How far the wind pushes a tip at this moment, in metres along WIND_DIRECTION. The
    // sine swings between -1 and 1. Its angle grows with time, starts at another point
    // for every tuft (the random number) and lags with the distance down the wind.
    float windPhase = uTime * WIND_SPEED + tuftRandom * TWO_PI -
                      dot(tuftRoot.xz, WIND_DIRECTION) * WIND_PHASE_PER_METRE;
    float windPush = uWindStrength * WIND_REACH * sin(windPhase);
    vec3 windOffset = vec3(WIND_DIRECTION.x, 0.0, WIND_DIRECTION.y) * windPush;

    for (int blade = 0; blade < BLADE_COUNT; ++blade) {
        // The direction this blade leans in, seen from above. The blades of a tuft are
        // spread evenly around a full turn, and the random number turns the whole tuft,
        // so no two tufts point the same way.
        float turn = (tuftRandom + float(blade) / float(BLADE_COUNT)) * TWO_PI;
        vec3 leanDirection = vec3(cos(turn), 0.0, sin(turn));
        // The flat side of the blade: across the lean direction, still horizontal.
        vec3 sideDirection = vec3(-leanDirection.z, 0.0, leanDirection.x);

        // A second random number, for the height of this blade. fract() keeps the part
        // after the decimal point: multiplying first and adding a different amount per
        // blade gives a number that has little to do with the first one.
        float bladeRandom =
            fract(tuftRandom * BLADE_RANDOM_FACTOR + float(blade) * BLADE_RANDOM_STEP);
        float height = uBladeHeight * mix(SHORTEST_BLADE, 1.0, bladeRandom);

        vec3 root = tuftRoot + leanDirection * ROOT_SPREAD;

        // The middle line of the blade at the three levels: root (0), half way
        // (HALF_LEVEL) and tip (1). The lean and the wind grow with the SQUARE of the
        // level: 0 at the root, a quarter half way up and all of it at the tip. So the
        // root stays where it is planted, the blade bends like a curve, and only the
        // upper part sways.
        vec3 middle = root + UP * (height * HALF_LEVEL) +
                      (leanDirection * (LEAN * height) + windOffset) * (HALF_LEVEL * HALF_LEVEL);
        vec3 tip = root + UP * height + leanDirection * (LEAN * height) + windOffset;

        // The width shrinks in a straight line: all of it at the root, half of it half
        // way up, nothing at the tip (one vertex).
        vec3 rootSide = sideDirection * ROOT_HALF_WIDTH;
        vec3 middleSide = sideDirection * (ROOT_HALF_WIDTH * (1.0 - HALF_LEVEL));

        // A triangle strip: every vertex after the second one makes a triangle with the
        // two before it. Left, right, left, right, tip gives three triangles.
        emitBladeVertex(root - rootSide, vec2(0.0, 0.0));
        emitBladeVertex(root + rootSide, vec2(1.0, 0.0));
        emitBladeVertex(middle - middleSide, vec2(0.0, HALF_LEVEL));
        emitBladeVertex(middle + middleSide, vec2(1.0, HALF_LEVEL));
        emitBladeVertex(tip, vec2(0.5, 1.0));
        // The strip of this blade ends here. The next blade starts a new one.
        EndPrimitive();
    }
}
