// Shadow mapping shared by lit.frag, gouraud.frag and grass.frag: the shadow maps of the
// moon and of the flashlight, and the functions that tell how much of the light of each
// a point loses.
// This file is not a shader of its own. It has no #version line: the shader loader puts
// its text in place of the line  #include "common/shadows.glsl"  (gfx/ShaderSource.hpp).
//
// The idea in two steps. Before the scene is drawn, everything that casts a shadow is
// drawn from the light into a depth texture, the shadow map (shadow_depth.vert): every
// texel then holds the depth of the surface nearest to the light. Here, for a fragment
// of the real picture, the same matrix gives its place in that map and its own depth as
// the light sees it. When the map holds a smaller depth, something lies between the
// light and the fragment: it is in shadow.

// The largest radius of the PCF kernel. The same number as game::MAX_PCF_RADIUS in
// src/game/Shadows.hpp.
const int MAX_PCF_RADIUS = 3;

// The shadow map of the moon, plain uniforms set from C++ for each program
// (game::setShadowUniforms). A sampler cannot be a member of a uniform block, so the
// numbers that belong to the map stay next to it instead of joining the light block.
// The moon has the texture unit 3 and the flashlight the unit 4
// (game/ShaderUniforms.hpp).
//
// sampler2DShadow: a depth texture that is read WITH A COMPARISON. texture() takes
// three numbers, the texture coordinate and a depth, and returns 1 where that depth is
// nearer to the light than the stored one (lit) and 0 where it is farther away (in
// shadow). The comparison and the rule for coordinates outside the map are set on the
// sampler object bound to the unit (gfx::ComparisonSampler). The uniform holds the
// number of that texture unit.
uniform sampler2DShadow uMoonShadowMap;
// World space to the clip space of the moon: the projection times the view the shadow
// map was drawn with (scene::LightSpace::matrix).
uniform mat4 uMoonShadowMatrix;
// Whether the map is read at all. Off: nothing is in shadow.
uniform bool uMoonShadowEnabled;
// The two parts of the bias, as differences of the depths the map stores (see
// slopeScaledBias).
uniform float uMoonShadowConstantBias;
uniform float uMoonShadowSlopeBias;
// Radius of the PCF kernel in texels: 0 is one comparison, 1 a square of 3 x 3, 2 of
// 5 x 5, 3 of 7 x 7.
uniform int uMoonShadowPcfRadius;
// The share of the moon light a shadow takes away: 1 leaves none of it, 0 all of it.
uniform float uMoonShadowStrength;

// The shadow map of the flashlight: the same set once more, with two differences. Its
// matrix holds a PERSPECTIVE projection (the light shines from one point), and the two
// parts of its bias are in METRES, not in stored depths: see flashlightShadow.
uniform sampler2DShadow uFlashlightShadowMap;
uniform mat4 uFlashlightShadowMatrix;
uniform bool uFlashlightShadowEnabled;
uniform float uFlashlightShadowConstantBias;
uniform float uFlashlightShadowSlopeBias;
uniform int uFlashlightShadowPcfRadius;
uniform float uFlashlightShadowStrength;
// Where the flashlight stands in world space: the place the shadow map was drawn from.
// It is the same point as uSpotPosition of the light block (C++ sets both from one
// result, game::flashlightPose). It is a uniform of its own because this file is also
// included by gouraud.frag, which does not have the light block.
uniform vec3 uFlashlightShadowLightPosition;

// The bias of a surface: how much nearer to the light it is taken to be in the
// comparison. A texel of the shadow map covers a small patch of a surface and stores
// one depth for it. On a tilted surface half of that patch is farther from the light
// than the stored depth and would count as being in its own shadow, in stripes ("shadow
// acne"). The steeper the tilt, the larger the difference, so the slope part grows as
// the surface turns away from the light.
// facing: cosine of the angle between the normal and the direction to the light.
// The same formula as game::shadowBias in C++.
float slopeScaledBias(float constantBias, float slopeBias, float facing) {
    return constantBias + slopeBias * (1.0 - clamp(facing, 0.0, 1.0));
}

// How much of a light reaches a point: 1 all of it, 0 none, in between at the soft edge
// of a shadow. coordinates is the place of the point in the shadow map: x and y the
// texture coordinate, z its depth as the light sees it, all from 0 to 1 inside the map.
// The function is written for any shadow map: the map is a parameter.
float shadowMapVisibility(sampler2DShadow map, vec3 coordinates, int pcfRadius) {
    // Farther away than the far plane of the light: the map knows nothing about this
    // place, and the comparison would cut the depth off at 1. Such a point is lit.
    // To the SIDE of the map no test is needed: there the sampler object returns its
    // border, a depth of 1, which nothing is behind.
    if (coordinates.z > 1.0) {
        return 1.0;
    }

    // One lookup. With the linear filter of the sampler object the graphics card
    // compares the four texels around the place and blends the four answers by itself.
    if (pcfRadius <= 0) {
        return texture(map, coordinates);
    }

    // Percentage closer filtering (PCF): the comparison is made for a square of texels
    // around the place, and the result is the share of them that are lit. A fragment at
    // the edge of a shadow gets a value between 0 and 1, and the edge turns soft.
    // Averaging the DEPTHS first and comparing once would not work: the average of the
    // depth of a wall and of the ground behind it is the depth of neither.
    //
    // textureSize gives the size of the map in texels, so one texel is 1 / size in
    // texture coordinates. textureOffset is not used: GLSL 4.10 wants its offset to be
    // a constant, and here it changes with the loop.
    int radius = min(pcfRadius, MAX_PCF_RADIUS);
    vec2 texel = 1.0 / vec2(textureSize(map, 0));
    float lit = 0.0;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            vec2 offset = vec2(float(x), float(y)) * texel;
            lit += texture(map, vec3(coordinates.xy + offset, coordinates.z));
        }
    }
    int side = 2 * radius + 1;
    return lit / float(side * side);
}

// The share of the moon light that does NOT reach a point, because something stands
// between it and the moon: 0 outside every shadow, uMoonShadowStrength in the middle of
// one. The caller takes that share of the moon light away (see lit.frag).
// worldPosition: the point in world space. facing: how much its surface faces the moon
// (moonFacing in common/lighting.glsl).
float moonShadow(vec3 worldPosition, float facing) {
    if (!uMoonShadowEnabled) {
        return 0.0;
    }

    // The steps of a vertex shader and of the graphics card after it, for the view of
    // the moon: the matrix gives clip space, the division by w normalised device
    // coordinates from -1 to 1 (w is 1 for the orthographic projection of the moon, so
    // it changes nothing here), and the last step the range from 0 to 1 that texture
    // coordinates and stored depths have. The same as scene::shadowMapCoordinates.
    vec4 clip = uMoonShadowMatrix * vec4(worldPosition, 1.0);
    vec3 coordinates = clip.xyz / clip.w * 0.5 + 0.5;

    // The bias: the point is compared as if it were this much nearer to the moon.
    coordinates.z -= slopeScaledBias(uMoonShadowConstantBias, uMoonShadowSlopeBias, facing);

    float visibility = shadowMapVisibility(uMoonShadowMap, coordinates, uMoonShadowPcfRadius);
    return uMoonShadowStrength * (1.0 - visibility);
}

// The share of the light of the flashlight that does NOT reach a point, because
// something stands between it and the flashlight: 0 outside every shadow,
// uFlashlightShadowStrength in the middle of one. The caller takes that share of the
// flashlight light away (see lit.frag).
// worldPosition: the point in world space. facing: how much its surface faces the
// flashlight (flashlightFacing in common/lighting.glsl).
float flashlightShadow(vec3 worldPosition, float facing) {
    if (!uFlashlightShadowEnabled) {
        return 0.0;
    }

    // The bias comes FIRST here, and in metres: the point is moved that far along the
    // straight line towards the flashlight, and the moved point is looked up in the
    // map. It stays on the same ray of the light, so it lands in the same texel, only
    // nearer to the light.
    //
    // Why not like the moon, by taking a number away from the depth: the projection of
    // the moon is orthographic and its stored depth grows evenly, so a metre is the
    // same step of depth everywhere. A perspective projection stores a depth that
    // changes fast near the light and hardly at all far from it (see linearDepth in
    // common/depth.glsl). One number taken from that depth would be centimetres next
    // to the flashlight and metres at the end of its beam. Moving the point in world
    // space, before the projection, is a bias of the same length everywhere.
    vec3 toLight = uFlashlightShadowLightPosition - worldPosition;
    float lightDistance = length(toLight);
    float bias = slopeScaledBias(uFlashlightShadowConstantBias, uFlashlightShadowSlopeBias, facing);
    // A point nearer to the flashlight than the bias would be moved past it. Nothing
    // can stand between the light and a point that close.
    if (lightDistance <= bias) {
        return 0.0;
    }
    vec3 biasedPosition = worldPosition + toLight / lightDistance * bias;

    // The matrix gives clip space. With a perspective projection w is the distance of
    // the point in front of the light, measured along the axis of its cone. At 0 or
    // below, the point is beside the light or behind it: the division would mirror it
    // into the map. Such a point is outside the cone and gets no light of the
    // flashlight anyway, so there is nothing a shadow could take away.
    vec4 clip = uFlashlightShadowMatrix * vec4(biasedPosition, 1.0);
    if (clip.w <= 0.0) {
        return 0.0;
    }

    // The perspective division, which really changes something here, and the step from
    // the range -1..1 to the range 0..1, as for the moon.
    vec3 coordinates = clip.xyz / clip.w * 0.5 + 0.5;

    float visibility =
        shadowMapVisibility(uFlashlightShadowMap, coordinates, uFlashlightShadowPcfRadius);
    return uFlashlightShadowStrength * (1.0 - visibility);
}
