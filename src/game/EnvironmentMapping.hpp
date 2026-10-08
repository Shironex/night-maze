// Environment mapping: the settings of the reflections of the sky on the crystals and
// the puddles, and the formulas of a reflected and of a refracted ray.
#pragma once

#include <glm/glm.hpp>

namespace game {

// Plain data and math without OpenGL, like the rest of the game_logic library, so tests
// can use it. The reflections themselves are computed for every fragment by the reflect
// program (assets/shaders/reflect.frag), which has the same formulas: the two files must
// agree.
//
// ENVIRONMENT MAPPING in one paragraph: a shiny surface shows its surroundings. Tracing
// rays through the scene to find out what it shows is too slow for a game, so the
// surroundings are taken from a picture that is already there: the cube map of the sky.
// For every fragment the shader computes the direction in which a ray from the eye
// leaves the surface (mirrored at it, or bent on the way into it) and reads the cube map
// in that direction. The cube map holds the sky and nothing else. So a crystal or
// a puddle shows the stars and the moon, and never the walls, the ground or the other
// crystals: that is the known limit of the technique.

/// How fast light travels in a material compared with empty space, as the number
/// physics calls the refractive index: 1 for air (and for empty space), more for
/// everything denser. Light bends where the index changes.
constexpr float AIR_REFRACTIVE_INDEX = 1.0F;
constexpr float WATER_REFRACTIVE_INDEX = 1.33F;
constexpr float GLASS_REFRACTIVE_INDEX = 1.5F;

/// The ratio a ray is bent with when it goes from air into glass: the index of the
/// material it leaves divided by the index of the material it enters, 1 / 1.5. This is
/// the number GLSL calls eta, the third argument of refract(). A crystal of quartz is
/// very close to glass (1.54).
constexpr float AIR_TO_GLASS_RATIO = AIR_REFRACTIVE_INDEX / GLASS_REFRACTIVE_INDEX;

/// The range of the refraction ratio the game accepts. 1 does not bend the ray at all.
/// Below 1 the ray enters a denser material (0.41 is air into diamond). Above 1 it would
/// leave a denser material, which a crystal seen from outside never does: the range goes
/// there only to show total internal reflection (see refractDirection).
constexpr float MIN_REFRACTION_RATIO = 0.4F;
constexpr float MAX_REFRACTION_RATIO = 1.5F;

/// The exponent of Schlick's formula (see fresnelSchlick).
constexpr float SCHLICK_EXPONENT = 5.0F;

/// The part of the free cells of a maze that gets a puddle at the start of the game,
/// and the largest part the slider offers: 0.15 gives the default maze 13 puddles in
/// its 85 free cells, and at 0.5 every second free cell has one.
constexpr float DEFAULT_PUDDLE_SHARE = 0.15F;
constexpr float MAX_PUDDLE_SHARE = 0.5F;

/// What can be changed about the environment mapping while the game runs. The debug UI
/// edits the fields. The values here are the defaults.
struct EnvironmentSettings {
    /// Whether anything reflects the sky. Switched off, the crystals are drawn like the
    /// walls (as they were before this effect existed) and the puddles are not drawn.
    bool enabled = true;

    /// How much of the colour of a crystal is the sky it shows: 0 leaves the lit
    /// crystal as it is, 1 replaces its lit colour by the sky. The glow of the crystal
    /// is added on top in both cases.
    float crystalStrength = 0.5F;

    /// What a crystal shows of the sky: 0 only the refracted picture (the sky seen
    /// THROUGH the crystal, like glass), 1 only the reflected one (the sky mirrored ON
    /// it, like metal), and a blend in between.
    float crystalReflectShare = 0.5F;

    /// The ratio the ray is bent with on its way into a crystal (eta of GLSL refract).
    float crystalRefractionRatio = AIR_TO_GLASS_RATIO;

    /// How much of its own glow a crystal keeps in the reflection pass: 1 all of it.
    /// The glow is several times brighter than the night sky, so at 1 the sky on
    /// a crystal is a faint tint. Lowering it shows the reflection and the refraction
    /// plainly, and takes the halo of the bloom away with the glow. The point light
    /// above the crystal is not changed by it.
    float crystalGlowShare = 1.0F;

    /// Whether the puddles are drawn.
    bool puddles = true;

    /// The part of the free cells of the maze that gets a puddle (see
    /// game::puddleCountFor). Changing it means choosing the cells again: the panel
    /// sets replacePuddles.
    float puddleShare = DEFAULT_PUDDLE_SHARE;

    /// How much of the colour of a puddle is the mirrored sky when the puddle is looked
    /// at straight from above: 0 none, 1 a perfect mirror. Real water has only 0.02
    /// there. The default is much higher on purpose: the night sky is dark, and with
    /// 0.02 a puddle a few steps away would show almost nothing of it. At 0.5 the stars
    /// can be made out in a puddle under the feet, and the rest of what is seen there
    /// is the water itself and the ground under it (a puddle is a thin film that lets
    /// the ground show through, see game::PuddleRenderer).
    float puddleReflectivity = 0.5F;

    /// Whether the mirror of a puddle gets stronger the flatter it is looked at
    /// (fresnelSchlick). Switched off, the reflectivity above is used at every angle.
    bool puddleFresnel = true;

    /// True when the share of the puddles changed and the puddles have not been placed
    /// again yet. The debug UI sets it, the application places them at the start of the
    /// next frame and clears it, like GrassSettings::replant.
    bool replacePuddles = false;
};

/// The direction of a ray after it is mirrored at a surface:
///
///     R = I - 2 * dot(N, I) * N
///
/// incident (I) is the direction the ray travels in, TOWARDS the surface. normal (N) is
/// the direction the surface faces, with length 1. dot(N, I) is how much of I goes
/// against the normal. Taking that part away once would leave the ray sliding along
/// the surface, taking it away twice sends it back out at the same angle. The result
/// is as long as incident. The same formula as the GLSL function reflect.
glm::vec3 reflectDirection(const glm::vec3& incident, const glm::vec3& normal);

/// The direction of a ray after it is bent on its way into a surface (refraction).
/// Snell's law: n1 * sin(angle before) = n2 * sin(angle after), with both angles
/// measured from the normal. ratio is n1 / n2 (GLSL: eta), the refractive index of the
/// material the ray leaves divided by the one it enters. The formula, with both vectors
/// of length 1:
///
///     k = 1 - ratio * ratio * (1 - dot(N, I) * dot(N, I))
///     T = ratio * I - (ratio * dot(N, I) + sqrt(k)) * N
///
/// k is the squared cosine of the angle after the bend. When it is below 0 no such
/// angle exists: the ray comes from the denser side at so flat an angle that it cannot
/// get out, and all of it is mirrored back in (total internal reflection). The result
/// is then the zero vector, exactly as the GLSL function refract returns it. A zero
/// vector is not a direction, so the caller has to handle it (refractOrReflect).
glm::vec3 refractDirection(const glm::vec3& incident, const glm::vec3& normal, float ratio);

/// refractDirection, or reflectDirection where the ray cannot be refracted (total
/// internal reflection). The result is always a direction a cube map can be read with.
/// The same function is in reflect.frag under the same name.
glm::vec3 refractOrReflect(const glm::vec3& incident, const glm::vec3& normal, float ratio);

/// How much of the light a smooth surface mirrors, from 0 to 1, depending on the angle
/// it is looked at (the Fresnel effect, in the approximation of Schlick):
///
///     F = F0 + (1 - F0) * (1 - cosine)^5
///
/// cosine is the cosine of the angle between the normal and the direction to the eye:
/// 1 looking straight down at the surface, 0 looking along it. straightOn (F0) is the
/// share that is mirrored when looking straight down. The flatter the look, the more is
/// mirrored, up to everything: this is why a puddle far ahead shows the sky and the
/// same puddle under the feet shows the ground below the water. cosine is clamped to
/// 0..1 first. The same function is in reflect.frag under the same name.
float fresnelSchlick(float cosine, float straightOn);

} // namespace game
