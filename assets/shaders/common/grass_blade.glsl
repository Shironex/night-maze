// The shape of a blade of grass, its sway in the wind and its distance LOD, shared by the
// two ways the blades are made: the geometry shader grass.geom (one point in, a tuft of
// blades out) and the vertex shader grass_instanced.vert (one small mesh of blades drawn
// once per tuft). Both include this file and call the same functions with the same
// numbers, so the two cannot drift apart: a blade looks the same whichever of them made
// it. This file is not a shader of its own. It has no #version line: the shader loader
// puts its text in place of the #include line (gfx/ShaderSource.hpp).

// Uniforms: set from C++ (game::GrassRenderer), the same for both programs.
uniform float uTime;         // seconds, always growing: the clock of the wind
uniform float uBladeHeight;  // height of the tallest blades in metres
uniform float uWindStrength; // 0: still air, 1: the default breeze
uniform bool uLodEnabled;    // whether far tufts lose blades (grassBladeScale)
uniform vec3 uEyePosition;   // the eye in world space, for the distance of a tuft
uniform float uLodNear;      // the third blade shrinks away behind this distance, metres
uniform float uLodFar;       // the second blade shrinks away behind this distance, metres
uniform float uLodFade;      // the width of the band a blade shrinks in, metres

const float TWO_PI = 6.2831853;
const vec3 UP = vec3(0.0, 1.0, 0.0);

// A tuft has BLADES_PER_TUFT blades, and a blade has VERTICES_PER_BLADE corners: two at
// the root, two half way up and one at the tip, which gives three triangles that get
// narrower towards the tip. game::GRASS_BLADES_PER_TUFT in C++ is the same number.
const int BLADES_PER_TUFT = 3;
const int VERTICES_PER_BLADE = 5;

// The numbers of the five corners, in the order of a triangle strip: left, right, left,
// right, tip. Every corner after the second one makes a triangle with the two before it.
const int ROOT_LEFT = 0;
const int ROOT_RIGHT = 1;
const int MIDDLE_LEFT = 2;
const int MIDDLE_RIGHT = 3;
const int TIP = 4;

// Half of the width of a blade at its root, in metres. The width shrinks to 0 at the tip.
// The four numbers below with a twin in C++ (game::GRASS_ROOT_HALF_WIDTH and the others
// in game/Grass.hpp, which make the box of a chunk) are checked against these by a test.
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
// grassBlade): what it is multiplied by, and what is added for every blade. Nothing
// depends on their exact values. They only must not be whole numbers, or simple
// fractions of each other.
const float BLADE_RANDOM_FACTOR = 7.0;
const float BLADE_RANDOM_STEP = 0.37;
// A third random number of the same kind, for the distance at which a tuft starts to
// lose a blade (grassBladeScale).
const float LOD_RANDOM_FACTOR = 13.0;

// The wind. It blows along WIND_DIRECTION (x and z of the world, length 1). The tip of
// a full blade swings WIND_REACH metres to each side at wind strength 1, WIND_SPEED
// radians of the swing pass per second, and a tuft one metre further down the wind is
// WIND_PHASE_PER_METRE radians behind: gusts travel over the grass like waves.
const vec2 WIND_DIRECTION = vec2(0.94, 0.34);
const float WIND_REACH = 0.06;
const float WIND_SPEED = 1.9;
const float WIND_PHASE_PER_METRE = 0.9;

// How far the wind pushes a tip at this moment, as a vector in world space. The whole
// tuft sways together, so it is computed once per tuft. The sine swings between -1 and
// 1. Its angle grows with time, starts at another point for every tuft (the random
// number) and lags with the distance down the wind.
vec3 grassWindOffset(vec3 tuftRoot, float tuftRandom) {
    float windPhase = uTime * WIND_SPEED + tuftRandom * TWO_PI -
                      dot(tuftRoot.xz, WIND_DIRECTION) * WIND_PHASE_PER_METRE;
    float windPush = uWindStrength * WIND_REACH * sin(windPhase);
    return vec3(WIND_DIRECTION.x, 0.0, WIND_DIRECTION.y) * windPush;
}

// What one blade of a tuft is: where its root stands, which way it leans, which way its
// flat side faces, and how tall it is.
struct GrassBlade {
    vec3 root;
    vec3 leanDirection;
    vec3 sideDirection;
    float height;
};

// Blade number blade (0 to BLADES_PER_TUFT - 1) of the tuft at tuftRoot.
GrassBlade grassBlade(vec3 tuftRoot, float tuftRandom, int blade) {
    GrassBlade result;
    // The direction this blade leans in, seen from above. The blades of a tuft are
    // spread evenly around a full turn, and the random number turns the whole tuft, so no
    // two tufts point the same way.
    float turn = (tuftRandom + float(blade) / float(BLADES_PER_TUFT)) * TWO_PI;
    result.leanDirection = vec3(cos(turn), 0.0, sin(turn));
    // The flat side of the blade: across the lean direction, still horizontal.
    result.sideDirection = vec3(-result.leanDirection.z, 0.0, result.leanDirection.x);

    // A second random number, for the height of this blade. fract() keeps the part after
    // the decimal point: multiplying first and adding a different amount per blade gives
    // a number that has little to do with the first one.
    float bladeRandom = fract(tuftRandom * BLADE_RANDOM_FACTOR + float(blade) * BLADE_RANDOM_STEP);
    result.height = uBladeHeight * mix(SHORTEST_BLADE, 1.0, bladeRandom);

    result.root = tuftRoot + result.leanDirection * ROOT_SPREAD;
    return result;
}

// How much of a blade is left at the distance of its tuft: 1 for the whole blade, 0 for
// none, and in between for a blade that is shrinking away. This is the distance LOD
// (level of detail): far away a blade is a pixel or less, and three blades of a tuft
// look like two, or like one. Fewer blades are fewer triangles and fewer tiny fragments
// for the card.
//
// The first blade always stays. The third one goes first, behind uLodNear, the second
// behind uLodFar. A blade does not vanish at once: it shrinks towards its root over
// a band of uLodFade metres. And not every tuft starts at the same distance: a third
// random number of the tuft moves its start somewhere into one more band of that width.
// So there is no line in the grass where the blades go, only a slow thinning out.
//
// The C++ side (game::grassBladesAt) knows the same rule: behind two bands nothing of
// the blade is left, so a chunk of tufts that are all that far away is drawn with fewer
// blades.
float grassBladeScale(vec3 tuftRoot, float tuftRandom, int blade) {
    if (!uLodEnabled || blade == 0) {
        return 1.0;
    }
    float start = (blade == 1 ? uLodFar : uLodNear) +
                  uLodFade * fract(tuftRandom * LOD_RANDOM_FACTOR);
    float distanceToEye = distance(uEyePosition, tuftRoot);
    return 1.0 - smoothstep(start, start + uLodFade, distanceToEye);
}

// The position of one corner of a blade in world space, and its blade coordinate
// (bladeUv.x: 0 at the left edge and 1 at the right edge, bladeUv.y: 0 at the root and
// 1 at the tip). windOffset is grassWindOffset of the tuft. scale is grassBladeScale:
// the whole blade is shrunk towards its root by it, so a scale of 0 makes all five
// corners one point and the blade disappears.
vec3 grassBladeCorner(GrassBlade blade, vec3 windOffset, int corner, float scale,
                      out vec2 bladeUv) {
    float height = blade.height;
    vec3 lean = blade.leanDirection * (LEAN * height);

    // Where the corner lies, measured from the root of the blade. The lean and the wind
    // grow with the SQUARE of the level: 0 at the root, a quarter half way up and all of
    // it at the tip. So the root stays where it is planted, the blade bends like a curve,
    // and only the upper part sways. The width shrinks in a straight line: all of it at
    // the root, half of it half way up, nothing at the tip (one vertex).
    vec3 offset;
    if (corner == TIP) {
        offset = UP * height + lean + windOffset;
        bladeUv = vec2(0.5, 1.0);
    } else if (corner == MIDDLE_LEFT || corner == MIDDLE_RIGHT) {
        float side = corner == MIDDLE_LEFT ? -1.0 : 1.0;
        offset = UP * (height * HALF_LEVEL) + (lean + windOffset) * (HALF_LEVEL * HALF_LEVEL) +
                 blade.sideDirection * (side * ROOT_HALF_WIDTH * (1.0 - HALF_LEVEL));
        bladeUv = vec2(corner == MIDDLE_LEFT ? 0.0 : 1.0, HALF_LEVEL);
    } else {
        float side = corner == ROOT_LEFT ? -1.0 : 1.0;
        offset = blade.sideDirection * (side * ROOT_HALF_WIDTH);
        bladeUv = vec2(corner == ROOT_LEFT ? 0.0 : 1.0, 0.0);
    }
    return blade.root + offset * scale;
}
