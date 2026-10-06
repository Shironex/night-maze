// Lighting shared by lit.frag and grass.frag (per fragment) and gouraud.vert (per
// vertex): the light block and the functions that turn the lights into the brightness of
// one surface point.
// This file is not a shader of its own. It has no #version line: the shader loader puts
// its text in place of the line  #include "common/lighting.glsl"  (gfx/ShaderSource.hpp).
// See docs/modules/scene/lights.md

// Length of the array of point lights. The same number as scene::MAX_POINT_LIGHTS in
// src/scene/Light.hpp.
const int MAX_POINT_LIGHTS = 16;

// One point light. vec4 everywhere, so that every member is 16 bytes and the C++ struct
// scene::PointLightData has the same layout without any hidden gaps.
struct PointLight {
    vec4 position;    // xyz: position in world space
    vec4 color;       // rgb: colour, a: intensity
    vec4 attenuation; // x: constant, y: linear, z: quadratic term
};

// All lights of the scene, in one uniform block. A block is not set uniform by uniform:
// it reads its bytes from a uniform buffer that the C++ code fills once per frame
// (gfx::UniformBuffer), and every program that declares the block sees the same data.
// std140 fixes the byte offset of every member, so the C++ struct scene::LightBlockData
// can mirror it. The members must stay in this order.
// GLSL 4.20 could name the binding point here, layout(std140, binding = 1). GLSL 4.10
// cannot, so the C++ code connects the block (gfx::Shader::bindUniformBlock).
layout(std140) uniform LightBlock {
    vec4 uCameraPosition;       // xyz: the eye in world space
    vec4 uAmbient;              // rgb: light that reaches every surface
    vec4 uDirectionalDirection; // xyz: the way the moon light travels, length 1
    vec4 uDirectionalColor;     // rgb: colour, a: intensity
    vec4 uSpotPosition;         // xyz: the flashlight in world space
    vec4 uSpotDirection;        // xyz: the axis of its cone, length 1
    vec4 uSpotColor;            // rgb: colour, a: intensity
    vec4 uSpotAttenuation;      // x: constant, y: linear, z: quadratic term
    vec4 uSpotCone;             // x: cos(inner angle), y: cos(outer angle), z: 1 on, 0 off
    int uPointCount;            // how many elements of uPoints are in use
    PointLight uPoints[MAX_POINT_LIGHTS];
};

// The material of the surface, plain uniforms set from C++ for each program.
// Which highlight formula to use. The numbers are the values of game::SpecularModel.
//   0: Phong, the reflected light ray compared with the direction to the eye
//   1: Blinn-Phong, the normal compared with the halfway vector
uniform int uSpecularModel;
// How bright the highlight is compared with the light that makes it (0: no highlight).
uniform float uSpecularStrength;
// The exponent of the highlight: a larger number gives a smaller, sharper highlight.
uniform float uShininess;

// The light that reaches one point of a surface, in two parts, because they are used
// differently: diffuse is multiplied by the colour of the surface (the texture), the
// highlight is added on top and keeps the colour of the light.
//
// The shares of the moon and of the flashlight are ALSO kept on their own. diffuse and
// specular already contain them. These are the two lights with a shadow map: where
// a surface lies in the shadow of one of them, the caller takes the share of THAT light
// away again (see common/shadows.glsl and the main function of lit.frag). Nothing else
// is ever taken away, so a shadow of the moon never darkens the flashlight, a shadow of
// the flashlight never darkens the moon light, and neither darkens the ambient light,
// the crystals or a glowing surface.
struct Lighting {
    vec3 diffuse;            // ambient light plus the Lambert term of every light
    vec3 specular;           // the highlight of every light
    vec3 moonDiffuse;        // the part of diffuse that comes from the moon
    vec3 moonSpecular;       // the part of specular that comes from the moon
    vec3 flashlightDiffuse;  // the part of diffuse that comes from the flashlight
    vec3 flashlightSpecular; // the part of specular that comes from the flashlight
};

// Lambert: a surface is brightest when it faces the light and gets darker as it turns
// away, with the cosine of the angle between its normal and the direction to the light.
// For two vectors of length 1 the dot product is that cosine. Below 0 the light is
// behind the surface: max() turns that into "no light" instead of negative light.
float diffuseFactor(vec3 normal, vec3 toLight) {
    return max(dot(normal, toLight), 0.0);
}

// The highlight: the mirror image of the light on a shiny surface. All three vectors
// have length 1 and point away from the surface point.
float specularFactor(vec3 normal, vec3 toLight, vec3 toEye) {
    // A surface that faces away from the light has no highlight either.
    if (dot(normal, toLight) <= 0.0) {
        return 0.0;
    }

    float cosine = 0.0;
    if (uSpecularModel == 0) {
        // Phong: reflect() mirrors the incoming ray (from the light to the surface,
        // hence the minus) at the normal. The highlight is strongest where the
        // mirrored ray goes straight into the eye.
        vec3 reflected = reflect(-toLight, normal);
        cosine = dot(reflected, toEye);
    } else {
        // Blinn-Phong: the halfway vector points exactly between the light and the
        // eye. The highlight is strongest where the normal points along it. The angle
        // measured here is about half of the Phong one, so with the same exponent the
        // highlight is wider, and it does not cut off when the light and the eye are
        // far apart (Phong: more than 90 degrees between reflected and toEye gives 0).
        vec3 halfway = normalize(toLight + toEye);
        cosine = dot(normal, halfway);
    }
    // max() before pow(): the power of a negative number is not defined in GLSL.
    return uSpecularStrength * pow(max(cosine, 0.0), uShininess);
}

// How much weaker a light is at the given distance (in metres). The same formula as
// scene::attenuationFactor in C++.
float attenuationFactor(vec4 terms, float lightDistance) {
    return 1.0 / (terms.x + terms.y * lightDistance + terms.z * lightDistance * lightDistance);
}

// How much a surface with this normal (length 1) faces the moon: the cosine of the angle
// between the normal and the direction to the moon. 1 facing it, 0 grazed by its light,
// below 0 facing away. The shadow bias grows as this number falls (common/shadows.glsl).
float moonFacing(vec3 normal) {
    return dot(normal, -uDirectionalDirection.xyz);
}

// The same for the flashlight: the cosine of the angle between the normal (length 1) of
// the surface at position and the direction from there to the flashlight. The moon is
// in the same direction from everywhere. The flashlight is a point, so the direction to
// it depends on where the surface is.
float flashlightFacing(vec3 normal, vec3 position) {
    return dot(normal, normalize(uSpotPosition.xyz - position));
}

// Adds one light to the result. radiance is the colour of the light as it arrives at the
// point: its colour times its intensity, already made weaker by distance and by the cone.
void addLight(inout Lighting lighting, vec3 normal, vec3 toLight, vec3 toEye, vec3 radiance) {
    lighting.diffuse += radiance * diffuseFactor(normal, toLight);
    lighting.specular += radiance * specularFactor(normal, toLight, toEye);
}

// The light at one point of a surface. position and normal are in world space, normal
// has length 1. lit.frag and grass.frag call this per fragment, gouraud.vert per vertex:
// the same function, and the lit and the gouraud program differ only in where it runs.
// This function knows nothing about shadows: it computes every light as if nothing stood
// in its way. The shadows of the moon and of the flashlight are applied by the caller,
// with the two moon fields and the two flashlight fields of the result.
Lighting computeLighting(vec3 position, vec3 normal) {
    vec3 toEye = normalize(uCameraPosition.xyz - position);

    Lighting lighting;
    lighting.diffuse = uAmbient.rgb;
    lighting.specular = vec3(0.0);

    // The moon, a directional light: the same direction everywhere, no attenuation.
    // uDirectionalDirection is the way the light travels, so the way TO the light is
    // the opposite. These lines do what addLight does, and keep the two terms.
    vec3 toMoon = -uDirectionalDirection.xyz;
    vec3 moonRadiance = uDirectionalColor.rgb * uDirectionalColor.a;
    lighting.moonDiffuse = moonRadiance * diffuseFactor(normal, toMoon);
    lighting.moonSpecular = moonRadiance * specularFactor(normal, toMoon, toEye);
    lighting.diffuse += lighting.moonDiffuse;
    lighting.specular += lighting.moonSpecular;

    // The point lights. The loop has a constant upper limit and leaves early. GLSL 4.10
    // does not require that (the rule comes from GLSL ES 1.00), but a constant limit
    // with an early exit is the form every compiler accepts.
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        if (i >= uPointCount) {
            break;
        }
        vec3 offset = uPoints[i].position.xyz - position;
        float lightDistance = length(offset);
        vec3 radiance = uPoints[i].color.rgb * uPoints[i].color.a *
                        attenuationFactor(uPoints[i].attenuation, lightDistance);
        addLight(lighting, normal, offset / lightDistance, toEye, radiance);
    }

    // The flashlight, a spot light: a point light that only shines into a cone. Its two
    // terms are kept like the ones of the moon. They are set to nothing first: a field
    // of a struct that was never written holds an undefined value, and the flashlight
    // may be switched off.
    lighting.flashlightDiffuse = vec3(0.0);
    lighting.flashlightSpecular = vec3(0.0);
    if (uSpotCone.z > 0.5) {
        vec3 offset = uSpotPosition.xyz - position;
        float lightDistance = length(offset);
        vec3 toLight = offset / lightDistance;

        // The cosine of the angle between the axis of the cone and the ray from the
        // light to this point. 1 on the axis, smaller towards the side.
        float cosAngle = dot(-toLight, uSpotDirection.xyz);
        // 1 inside the inner cone, 0 outside the outer one, a ramp in between: the soft
        // edge. The same formula as scene::spotFactor. The C++ code makes sure that the
        // two cosines differ, so this never divides by zero.
        float cone = clamp((cosAngle - uSpotCone.y) / (uSpotCone.x - uSpotCone.y), 0.0, 1.0);

        vec3 radiance = uSpotColor.rgb * uSpotColor.a * cone *
                        attenuationFactor(uSpotAttenuation, lightDistance);
        // What addLight does, with the two terms kept, as for the moon.
        lighting.flashlightDiffuse = radiance * diffuseFactor(normal, toLight);
        lighting.flashlightSpecular = radiance * specularFactor(normal, toLight, toEye);
        lighting.diffuse += lighting.flashlightDiffuse;
        lighting.specular += lighting.flashlightSpecular;
    }

    return lighting;
}
