#version 410 core
// Fragment shader of the surfaces that show the sky (crystals and puddles): the lit
// colour of the surface, blended with the sky read from the cube map in the direction
// of the mirrored ray and of the refracted ray (environment mapping).
// See docs/modules/renderer/env-mapping.md

// The light block and the function computeLighting: the same file lit.frag includes, so
// a crystal is lit here exactly as lit.frag lit it.
#include "common/lighting.glsl"
// The normal map and the function surfaceNormal, as in lit.frag.
#include "common/normal_map.glsl"
// The shadow maps of the moon and of the flashlight, as in lit.frag.
#include "common/shadows.glsl"

// Inputs from the vertex shader, already blended for this fragment.
in vec2 vUv;            // texture coordinate
in vec3 vNormal;        // normal in world space, no longer exactly of length 1
in vec3 vTangent;       // tangent in world space, no longer exactly of length 1
in vec3 vWorldPosition; // position in world space

// The texture, the colour of the material and the glow of the surface, as in lit.frag.
// A puddle has a plain white texture and the colour of its water as its tint.
uniform sampler2D uTexture;
uniform vec3 uTint;
uniform vec3 uEmissive;

// Whether the surface is lit by the lights of the scene (1) or shown at full brightness
// (0). It is off in the lighting mode Unlit, as in grass.frag.
uniform bool uLit;

// The six pictures of the sky: the same cube map the skybox is drawn from. A
// samplerCube holds the NUMBER OF A TEXTURE UNIT, set from C++ (gfx::Shader::setInt),
// and is read with a direction instead of (u, v), see skybox.frag.
uniform samplerCube uEnvironmentMap;
// The number the colour of the sky is multiplied by: the brightness of the skybox, so
// the mirrored sky is as bright as the sky behind the walls.
uniform float uSkyBrightness;
// Whether there is a sky to show (1) or not (0): the skybox is switched off, or its
// pictures could not be loaded. Without a sky the background of the frame is one colour
// (the clear colour), and that colour is what a mirror shows then, in every direction.
uniform bool uSkyVisible;
uniform vec3 uBackground; // the clear colour, a linear colour

// How much of the colour of the surface is the sky it shows, from 0 (the lit surface
// alone) to 1 (the sky alone). With uFresnelEnabled it is the share for a look straight
// down at the surface, and the share grows as the look gets flatter.
uniform float uEnvironmentStrength;
uniform bool uFresnelEnabled;
// What the surface shows of the sky: 0 the refracted picture only (seen through the
// surface), 1 the mirrored picture only.
uniform float uReflectShare;
// The ratio the ray is bent with on its way into the surface: the refractive index of
// the material it leaves divided by the one it enters (air into glass: 1 / 1.5).
uniform float uRefractionRatio;

// The soft rim of a puddle. uRimFade is the part of the radius over which the water
// fades out towards the rim, and uOpacity how much the water hides of the ground in
// the middle of the puddle. With uRimFade at 0 (the crystals, and the value after
// a reload) the surface is solid and uOpacity is not read.
uniform float uRimFade;
uniform float uOpacity;

// Output: the color written to the HDR framebuffer of the scene (red, green, blue,
// alpha), as a LINEAR colour that may be brighter than 1.
out vec4 fragColor;

// The texture coordinate in the middle of a puddle and the distance from there to its
// rim (game::buildPuddleMesh).
const vec2 PUDDLE_UV_CENTER = vec2(0.5);
const float PUDDLE_UV_RADIUS = 0.5;

// The exponent of Schlick's formula. The same number as game::SCHLICK_EXPONENT in
// src/game/EnvironmentMapping.hpp.
const float SCHLICK_EXPONENT = 5.0;

// The direction of a ray after it is bent on its way into the surface, or, where it
// cannot be bent, after it is mirrored. incident is the direction the ray travels in,
// towards the surface, and normal the direction the surface faces, both of length 1.
// The same function as game::refractOrReflect in C++.
//
// refract() is Snell's law: n1 * sin(angle before) = n2 * sin(angle after), with
// ratio = n1 / n2. Written out it is
//     k = 1 - ratio * ratio * (1 - dot(N, I) * dot(N, I))
//     T = ratio * I - (ratio * dot(N, I) + sqrt(k)) * N
// and for k below 0 it returns the vector (0, 0, 0): the ray comes from the denser side
// at so flat an angle that none of it gets through (total internal reflection). A zero
// vector is no direction, and a cube map read with it gives an undefined colour. All of
// that light is mirrored, so the mirrored ray is the right answer there.
vec3 refractOrReflect(vec3 incident, vec3 normal, float ratio) {
    vec3 refracted = refract(incident, normal, ratio);
    if (refracted == vec3(0.0)) {
        return reflect(incident, normal);
    }
    return refracted;
}

// How much of the light a smooth surface mirrors, from 0 to 1 (the Fresnel effect in
// the approximation of Schlick). cosine is the cosine of the angle between the normal
// and the direction to the eye, straightOn the share for a look straight down at the
// surface. The flatter the look, the more is mirrored. The same function as
// game::fresnelSchlick in C++.
float fresnelSchlick(float cosine, float straightOn) {
    float facing = clamp(cosine, 0.0, 1.0);
    return straightOn + (1.0 - straightOn) * pow(1.0 - facing, SCHLICK_EXPONENT);
}

// What the surroundings look like in one direction of the world: the sky, as bright as
// the skybox draws it. The cube map is an sRGB texture, so the value is already
// a linear colour (see skybox.frag). The surroundings are the sky and NOTHING ELSE:
// a wall that stands in that direction is not in the cube map, so it is not mirrored.
vec3 environmentColor(vec3 direction) {
    if (!uSkyVisible) {
        return uBackground;
    }
    return texture(uEnvironmentMap, direction).rgb * uSkyBrightness;
}

void main() {
    // The lit colour of the surface, computed exactly as in lit.frag (see the comments
    // there): the normal, the light of every lamp, and the shadows of the moon and of
    // the flashlight taken away from the light of those two.
    vec3 normal = surfaceNormal(vNormal, vTangent, vUv);

    // In the mode Unlit the surface is shown as if lit by white light of strength 1 and
    // without a highlight, as in textured.frag.
    vec3 diffuse = vec3(1.0);
    vec3 specular = vec3(0.0);
    if (uLit) {
        Lighting lighting = computeLighting(vWorldPosition, normal);
        vec3 modelNormal = normalize(vNormal);
        float shadow = moonShadow(vWorldPosition, moonFacing(modelNormal));
        float flashlightShade =
            flashlightShadow(vWorldPosition, flashlightFacing(modelNormal, vWorldPosition));
        diffuse = max(lighting.diffuse - lighting.moonDiffuse * shadow -
                          lighting.flashlightDiffuse * flashlightShade,
                      0.0);
        specular = max(lighting.specular - lighting.moonSpecular * shadow -
                           lighting.flashlightSpecular * flashlightShade,
                       0.0);
    }
    vec3 surface = texture(uTexture, vUv).rgb * uTint;
    vec3 litColor = surface * diffuse + specular;

    // ENVIRONMENT MAPPING. The incident ray I: the direction from the eye to this
    // fragment, in world space. uCameraPosition is the eye, from the light block.
    vec3 incident = normalize(vWorldPosition - uCameraPosition.xyz);

    // The mirrored ray: R = I - 2 * dot(N, I) * N, the GLSL function reflect. The part
    // of I that goes against the normal is turned around, the rest stays.
    vec3 reflected = reflect(incident, normal);
    // The ray that enters the surface, bent by Snell's law. Only this one bend is
    // followed: a real ray would be bent again where it leaves the crystal at the back,
    // and that second surface is unknown here. The picture still reads as glass.
    vec3 refracted = refractOrReflect(incident, normal, uRefractionRatio);

    // The sky in both directions, blended: uReflectShare 0 shows the sky through the
    // surface, 1 the sky mirrored on it.
    vec3 environment =
        mix(environmentColor(refracted), environmentColor(reflected), uReflectShare);

    // How much of the sky is shown. With the Fresnel effect it depends on the angle:
    // dot(N, -I) is the cosine of the angle between the normal and the direction to the
    // eye, 1 looking straight at the surface and 0 looking along it.
    float strength = uEnvironmentStrength;
    if (uFresnelEnabled) {
        strength = fresnelSchlick(dot(normal, -incident), uEnvironmentStrength);
    }

    // The sky replaces a share of the lit colour. The glow of the surface itself
    // (uEmissive, see lit.frag) is added afterwards and so stays whole: a crystal keeps
    // glowing, and the bloom keeps finding it, however much of the sky it shows. With
    // a strength of 0 this line gives exactly the colour of lit.frag.
    vec3 color = mix(litColor, environment, strength) + surface * uEmissive;

    // THE ALPHA: how much of this colour covers what is in the framebuffer already. It
    // matters only while OpenGL blends, which game::PuddleRenderer switches on for the
    // puddles alone. A crystal is solid: 1.
    float alpha = 1.0;
    if (uRimFade > 0.0) {
        // How far out this fragment lies in its puddle: 0 in the middle, 1 at the rim.
        // It is computed per fragment from the texture coordinate, so the fade is
        // a true circle whatever the number of corners of the mesh.
        float outwards = length(vUv - PUDDLE_UV_CENTER) / PUDDLE_UV_RADIUS;
        // smoothstep goes from 0 to 1 between its first two arguments, slowly at both
        // ends: the water is whole up to (1 - uRimFade) of the radius and gone at the
        // rim, without a visible line where the fade starts or ends.
        float gone = smoothstep(1.0 - uRimFade, 1.0, outwards);
        // The mirror gets stronger at flat angles (Fresnel), and so does the cover: far
        // ahead the water hides the ground, under the feet the ground shows through.
        alpha = mix(uOpacity, 1.0, strength) * (1.0 - gone);
    }
    fragColor = vec4(color, alpha);
}
