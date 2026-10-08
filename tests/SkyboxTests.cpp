// Tests of the sky pictures in assets/skybox: they load as the faces of a cube map, the
// moon is painted where the moon light comes from, and the faces meet without seams.
#include "assets/ImageLoader.hpp"
#include "game/Lighting.hpp"
#include "scene/Light.hpp"

#include <doctest/doctest.h>
#include <glm/glm.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <string>

namespace {

// The assets directory of the repository, see ImageLoaderTests.cpp.
std::filesystem::path assetsDirectory() {
    return NIGHT_MAZE_ASSETS_DIR;
}

constexpr std::size_t FACE_COUNT = 6;

// The numbers of the faces, in the order OpenGL and gfx::Cubemap use.
constexpr std::size_t POSITIVE_X = 0;
constexpr std::size_t NEGATIVE_X = 1;
constexpr std::size_t POSITIVE_Y = 2;
constexpr std::size_t NEGATIVE_Y = 3;
constexpr std::size_t POSITIVE_Z = 4;
constexpr std::size_t NEGATIVE_Z = 5;

// The files of the faces in that order, as game::Skybox loads them.
constexpr std::array<const char*, FACE_COUNT> FACE_FILES = {
    "px.png", "nx.png", "py.png", "ny.png", "pz.png", "nz.png",
};

using SkyFaces = std::array<assets::Image, FACE_COUNT>;

// Loads the six faces the way the game does: top row first, no flip.
SkyFaces loadSkyFaces() {
    SkyFaces faces;
    for (std::size_t face = 0; face < FACE_COUNT; ++face) {
        std::string error;
        REQUIRE(assets::loadImage(assetsDirectory() / "skybox" / FACE_FILES[face], faces[face],
                                  error, assets::RowOrder::TopFirst));
    }
    return faces;
}

// Where a direction hits a cube map: which face, and the coordinates s and t on it, both
// from 0 to 1.
struct FacePoint {
    std::size_t face = 0;
    float s = 0.0F;
    float t = 0.0F;
};

// The rule the graphics card reads a cube map with, copied from the table "Selection of
// cube map images" of the OpenGL specification: the axis on which the direction is
// longest picks the face, and the other two components, divided by that length, are the
// place on the face. The test has its own copy of the rule on purpose: it checks the
// pictures against OpenGL, not against the script that made them.
FacePoint facePointOf(const glm::vec3& direction) {
    const glm::vec3 length = glm::abs(direction);
    std::size_t face = 0;
    float sc = 0.0F; // grows to the right of the face picture
    float tc = 0.0F; // grows DOWN the face picture
    float major = 0.0F;
    if (length.x >= length.y && length.x >= length.z) {
        face = direction.x > 0.0F ? POSITIVE_X : NEGATIVE_X;
        sc = direction.x > 0.0F ? -direction.z : direction.z;
        tc = -direction.y;
        major = length.x;
    } else if (length.y >= length.z) {
        face = direction.y > 0.0F ? POSITIVE_Y : NEGATIVE_Y;
        sc = direction.x;
        tc = direction.y > 0.0F ? direction.z : -direction.z;
        major = length.y;
    } else {
        face = direction.z > 0.0F ? POSITIVE_Z : NEGATIVE_Z;
        sc = direction.z > 0.0F ? direction.x : -direction.x;
        tc = -direction.y;
        major = length.z;
    }
    return {.face = face, .s = (sc / major + 1.0F) / 2.0F, .t = (tc / major + 1.0F) / 2.0F};
}

// The colour (red, green, blue from 0 to 255) of the sky in a direction: the nearest
// pixel of the face the direction hits. t = 0 is the first row in memory, which with
// RowOrder::TopFirst is the top row of the picture.
glm::vec3 skyColor(const SkyFaces& faces, const glm::vec3& direction) {
    const FacePoint point = facePointOf(direction);
    const assets::Image& image = faces[point.face];
    const int column =
        std::clamp(static_cast<int>(point.s * static_cast<float>(image.width)), 0, image.width - 1);
    const int row = std::clamp(static_cast<int>(point.t * static_cast<float>(image.height)), 0,
                               image.height - 1);
    const std::size_t first =
        (static_cast<std::size_t>(row) * static_cast<std::size_t>(image.width) +
         static_cast<std::size_t>(column)) *
        static_cast<std::size_t>(image.channels);
    return {image.pixels[first], image.pixels[first + 1], image.pixels[first + 2]};
}

// The brightness of a colour as one number: the average of its three channels.
float brightness(const glm::vec3& color) {
    return (color.r + color.g + color.b) / 3.0F;
}

// The unit vector along one axis (0 is x, 1 is y, 2 is z), towards plus or minus.
glm::vec3 axisDirection(int axis, float sign) {
    glm::vec3 direction{0.0F};
    direction[axis] = sign;
    return direction;
}

} // namespace

TEST_CASE("the six sky pictures are squares of one size with three channels") {
    const SkyFaces faces = loadSkyFaces();
    for (std::size_t face = 0; face < FACE_COUNT; ++face) {
        CAPTURE(face);
        CHECK(faces[face].width == faces[0].width);
        CHECK(faces[face].height == faces[0].width);
        CHECK(faces[face].channels == 3);
    }
    // Small faces would be blurred on the screen: one face covers 90 degrees.
    CHECK(faces[0].width >= 512);
}

TEST_CASE("the rule of the cube map faces: the middle and the corners of a face") {
    // A direction along an axis hits the middle of that face.
    const FacePoint front = facePointOf({0.0F, 0.0F, -1.0F});
    CHECK(front.face == NEGATIVE_Z);
    CHECK(front.s == doctest::Approx(0.5F));
    CHECK(front.t == doctest::Approx(0.5F));

    // A direction up and to the right for a camera that looks along -Z. Up is the top
    // of the picture (t = 0). But +X, the right of the screen, is in the LEFT half of
    // the picture (s below 0.5): a face picture shows its side of the cube as seen from
    // OUTSIDE, so from the inside it is mirrored left to right. That is why the files
    // in assets/skybox look mirrored next to a screenshot, and it is correct.
    const FacePoint frontUpRight = facePointOf({0.5F, 0.5F, -1.0F});
    CHECK(frontUpRight.face == NEGATIVE_Z);
    CHECK(frontUpRight.s == doctest::Approx(0.25F));
    CHECK(frontUpRight.t == doctest::Approx(0.25F));

    // Straight up is the top face. Its picture has +Z at the bottom (t = 1).
    const FacePoint upBack = facePointOf({0.0F, 1.0F, 0.5F});
    CHECK(upBack.face == POSITIVE_Y);
    CHECK(upBack.s == doctest::Approx(0.5F));
    CHECK(upBack.t == doctest::Approx(0.75F));
}

TEST_CASE("the moon is painted where the default moon light comes from") {
    const SkyFaces faces = loadSkyFaces();

    // The two angles say which way the light TRAVELS. It comes from the opposite
    // direction, and that is where the disc has to be. This is the coupling between
    // game::LightingSettings and tools/blender/make_skybox.py: when the defaults change
    // and the sky is not generated again, this test fails.
    const game::LightingSettings defaults;
    const glm::vec3 lightTravels =
        scene::directionFromAngles(defaults.moonYawDegrees, defaults.moonPitchDegrees);
    const glm::vec3 toMoon = -lightTravels;

    // The middle of the disc is nearly white.
    const glm::vec3 disc = skyColor(faces, toMoon);
    CHECK(disc.r > 150.0F);
    CHECK(disc.g > 150.0F);
    CHECK(disc.b > 150.0F);

    // The disc is small. A quarter of the sky away, and in the direction the light
    // travels to (below the horizon), the sky is dark: single stars may be brighter, the
    // average of many directions is not. The directions go around the moon in a cone.
    constexpr int SAMPLE_COUNT = 360;
    const glm::vec3 side = glm::normalize(glm::cross(toMoon, glm::vec3{0.0F, 1.0F, 0.0F}));
    const glm::vec3 other = glm::cross(toMoon, side);
    float sum = 0.0F;
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        const float angle = glm::radians(static_cast<float>(i));
        sum += brightness(skyColor(faces, side * std::cos(angle) + other * std::sin(angle)));
    }
    CHECK(sum / static_cast<float>(SAMPLE_COUNT) < 40.0F);
    CHECK(brightness(skyColor(faces, lightTravels)) < 40.0F);
}

TEST_CASE("the sky is slightly lighter at the horizon than straight above") {
    const SkyFaces faces = loadSkyFaces();

    // Averages over many directions, so that single stars do not decide the result: a
    // ring 10 degrees around the zenith and a ring just above the horizon. Both rings
    // stay away from the moon (50 degrees high) and its halo.
    constexpr int SAMPLE_COUNT = 720;
    constexpr float ZENITH_RING_DEGREES = 80.0F;
    constexpr float HORIZON_RING_DEGREES = 3.0F;
    float zenith = 0.0F;
    float horizon = 0.0F;
    for (int i = 0; i < SAMPLE_COUNT; ++i) {
        const float yaw = 360.0F * static_cast<float>(i) / static_cast<float>(SAMPLE_COUNT);
        zenith += brightness(skyColor(faces, scene::directionFromAngles(yaw, ZENITH_RING_DEGREES)));
        horizon +=
            brightness(skyColor(faces, scene::directionFromAngles(yaw, HORIZON_RING_DEGREES)));
    }
    CHECK(horizon > zenith);
    // Still a night sky: a backdrop, far darker than the lit walls.
    CHECK(horizon / static_cast<float>(SAMPLE_COUNT) < 60.0F);
}

TEST_CASE("neighbouring sky faces show the same sky on both sides of their border") {
    const SkyFaces faces = loadSkyFaces();

    // Two faces of different axes share an edge of the cube: 12 edges in all. Along an
    // edge the directions are a + b + u * c, where a and b are the axes of the two faces
    // and c is the third axis, with u from -1 to 1. Stretching a by a little picks the
    // last pixels of the first face, stretching b the last pixels of the second one.
    // Those two rows of pixels lie next to each other in the sky, so on average they
    // must have nearly the same colour. A face that is upside down, mirrored or stored
    // under the name of another face breaks this on its borders: the horizon, the Milky
    // Way and the halo of the moon no longer continue.
    constexpr int SAMPLE_COUNT = 1000;
    constexpr float NUDGE = 1.001F;
    // In levels of an 8-bit channel. Measured on the generated pictures: 0.3 to 0.9 on
    // the twelve borders (the grain of the dithering and the stars that lie on a border).
    // With all six faces loaded upside down (the row flip of 2D textures), the eight
    // borders of the top and the bottom face measure 3.9 to 7.1.
    constexpr float MAX_AVERAGE_DIFFERENCE = 2.0F;

    for (int firstAxis = 0; firstAxis < 3; ++firstAxis) {
        for (int secondAxis = firstAxis + 1; secondAxis < 3; ++secondAxis) {
            // The three axes are 0, 1 and 2, so the third one is what is left of their sum.
            const int thirdAxis = 3 - firstAxis - secondAxis;
            for (const float firstSign : {-1.0F, 1.0F}) {
                for (const float secondSign : {-1.0F, 1.0F}) {
                    CAPTURE(firstAxis);
                    CAPTURE(secondAxis);
                    CAPTURE(firstSign);
                    CAPTURE(secondSign);
                    const glm::vec3 a = axisDirection(firstAxis, firstSign);
                    const glm::vec3 b = axisDirection(secondAxis, secondSign);
                    const glm::vec3 c = axisDirection(thirdAxis, 1.0F);

                    float difference = 0.0F;
                    for (int i = 0; i < SAMPLE_COUNT; ++i) {
                        // From -0.999 to 0.999: every pixel along the edge but the corners.
                        const float u = (static_cast<float>(i) + 0.5F) /
                                            static_cast<float>(SAMPLE_COUNT) * 2.0F -
                                        1.0F;
                        const glm::vec3 onFirst = skyColor(faces, a * NUDGE + b + c * u);
                        const glm::vec3 onSecond = skyColor(faces, a + b * NUDGE + c * u);
                        const glm::vec3 apart = glm::abs(onFirst - onSecond);
                        difference += (apart.r + apart.g + apart.b) / 3.0F;
                    }
                    CHECK(difference / static_cast<float>(SAMPLE_COUNT) < MAX_AVERAGE_DIFFERENCE);
                }
            }
        }
    }
}
