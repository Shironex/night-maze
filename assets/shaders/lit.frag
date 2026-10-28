#version 410 core
// Fragment shader of lit models, lighting per fragment (Phong shading): the light is
// computed for every fragment from its own position and normal. The highlight formula
// (Phong or Blinn-Phong) is chosen by the uniform uSpecularModel.

// The light block and the function computeLighting. The same file is included by
// gouraud.vert and grass.frag.
#include "common/lighting.glsl"
// The normal map and the function surfaceNormal. The same file is included by
// textured.frag, for its debug view of the normals.
#include "common/normal_map.glsl"
// The shadow maps of the moon and of the flashlight and the functions moonShadow and
// flashlightShadow. The same file is included by gouraud.frag and grass.frag.
#include "common/shadows.glsl"

// Inputs from the vertex shader, already blended for this fragment.
in vec2 vUv;            // texture coordinate
in vec3 vNormal;        // normal in world space, no longer exactly of length 1
in vec3 vTangent;       // tangent in world space, no longer exactly of length 1
in vec3 vWorldPosition; // position in world space

// The texture (the number of a texture unit) and the colour of the material, as in
// textured.frag.
uniform sampler2D uTexture;
uniform vec3 uTint;

// Light the surface gives off by itself, as a colour that multiplies the colour of the
// surface. Black (0, 0, 0) for everything that only reflects light: walls, ground,
// pillars, the gate. The crystals glow with it: the point light of a crystal hangs
// outside its mesh and lights its faces only from one side, and without a glow of its
// own the source of the light would be the darkest thing around it.
uniform vec3 uEmissive;

// The land around the maze only (game::setLandLight), black for everything else. The
// land lies far out, where the moon alone leaves a hill a flat dark shape against the sky.
// uSkyFill is light from the sky, more on a face that looks up than on one that leans
// away, so the slopes of a hill part from each other. uRimLight lights a face the more the
// eye sees it edge-on, so the outline of a crown or a roof catches a little light against
// the sky. Both grow from nothing at the border of the maze (the box from uLandFrom to
// uLandTo, x and z) to all of it uLandFade metres out, so the ground of the maze and the
// land meet without a step. With uLandFade at 0 there is none of either.
uniform vec3 uSkyFill;
uniform vec3 uRimLight;
uniform vec3 uLandFrom;
uniform vec3 uLandTo;
uniform float uLandFade;

// How much of the light of the land a point gets: 0 in the maze, 1 from uLandFade out.
float landShare(vec3 position) {
    if (uLandFade <= 0.0) {
        return 0.0;
    }
    vec2 outside = max(max(uLandFrom.xz - position.xz, position.xz - uLandTo.xz), 0.0);
    return smoothstep(0.0, uLandFade, length(outside));
}

// Output: the color written to the HDR framebuffer of the scene (red, green, blue,
// alpha), as a LINEAR colour that may be brighter than 1.
out vec4 fragColor;

void main() {
    // The normal of this fragment: the one of the model, or with normal mapping the one
    // read from the normal map. This is the only place where normal mapping enters the
    // lighting: the formulas of computeLighting do not know where their normal comes
    // from. It needs a normal per fragment, which is why the Gouraud program (light per
    // vertex) has no normal mapping.
    vec3 normal = surfaceNormal(vNormal, vTangent, vUv);
    Lighting lighting = computeLighting(vWorldPosition, normal);

    // The shadow of the moon: the share of the moon light that something on the way
    // from the moon keeps from this fragment, from 0 (none of it) to 1 (all of it).
    // That share is taken away from the light again, and only the moon light is: the
    // ambient light, the flashlight and the crystals shine in a moon shadow as they do
    // outside it. With the shadows switched off the share is 0 and nothing changes.
    //
    // The bias of the shadow depends on how the surface is tilted against the moon. It
    // is asked with the normal of the MODEL, not the one of the normal map: the bias
    // belongs to the triangle that was drawn into the shadow map, and the bumps of
    // a normal map are not in that triangle.
    //
    // max(): the difference is never below 0 on paper, but two floats that should be
    // equal can differ in their last digit.
    vec3 modelNormal = normalize(vNormal);
    float shadow = moonShadow(vWorldPosition, moonFacing(modelNormal));

    // The shadow of the flashlight, in the same way and from a map of its own: the
    // share of the flashlight light that something on the way from the hand of the
    // player keeps from this fragment. Only the light of the flashlight is taken away
    // by it. A fragment in the shadow of the moon AND of the flashlight loses both
    // shares and keeps the ambient light and the crystals.
    float flashlightShade =
        flashlightShadow(vWorldPosition, flashlightFacing(modelNormal, vWorldPosition));

    vec3 diffuse = max(lighting.diffuse - lighting.moonDiffuse * shadow -
                           lighting.flashlightDiffuse * flashlightShade,
                       0.0);
    vec3 specular = max(lighting.specular - lighting.moonSpecular * shadow -
                            lighting.flashlightSpecular * flashlightShade,
                        0.0);

    // The colour of the surface takes part in the diffuse light only: a red wall
    // reflects the red part of the light. The highlight is added on top in the colour
    // of the light, as in the Phong model of the lecture.
    //
    // Everything here is linear, which is what makes multiplying by a light and adding
    // lights correct: the texture is an sRGB texture that the graphics card decodes on
    // reading, the light colours were converted in C++ (game::buildLightSet), and the
    // result is written as it is into a floating point buffer, also when it is above
    // 1. Exposure, tone mapping and the sRGB encoding (gamma correction) follow once,
    // for the whole frame, in post/composite.frag.
    //
    // The glow of the surface itself (uEmissive) joins the diffuse light. It does not
    // depend on any light of the scene, so a crystal glows in the darkest corner too.
    vec3 toEye = normalize(uCameraPosition.xyz - vWorldPosition);
    float edge = 1.0 - max(dot(normal, toEye), 0.0);
    diffuse += landShare(vWorldPosition) *
               (uSkyFill * (0.5 + 0.5 * normal.y) + uRimLight * (edge * edge * edge));

    vec3 surface = texture(uTexture, vUv).rgb * uTint;
    fragColor = vec4(surface * (diffuse + uEmissive) + specular, 1.0);
}
