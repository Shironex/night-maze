// Tests of game/Storybook.hpp: reading the manifest of the storybook intro, how long its
// pictures stay, what it shows at a moment and where its layers are.
#include "game/Storybook.hpp"

#include "core/Files.hpp"
#include "gfx/CoverFit.hpp"

#include <doctest/doctest.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace {

using game::StorybookFrame;

// Plays the storybook in frames of frameSeconds, the way the application does, and
// returns the pictures whose spoken lines were started, in order.
std::vector<std::size_t> voicesOfRun(const std::vector<float>& beats, float frameSeconds) {
    std::vector<std::size_t> played;
    float clock = 0.0F;
    while (clock < game::storybookSeconds(beats) + 2.0F) {
        const float next = clock + frameSeconds;
        for (const std::size_t beat : game::storybookVoicesBetween(beats, clock, next)) {
            played.push_back(beat);
        }
        clock = next;
    }
    return played;
}

} // namespace

TEST_CASE("a manifest is read with its music, beats, lines, voices and layers") {
    const game::StorybookResult result =
        game::parseStorybook("# the intro\r\n"
                             "music theme.flac\n"
                             "crossfade 2\n"
                             "\n"
                             "beat\n"
                             "  seconds 7.5\n"
                             "  voice beat1.wav\n"
                             "  line Every winter the moon.\n"
                             "  line It falls.\n"
                             "  layer sky.png\n"
                             "  layer ridge.png -0.02 0.01 1.05 1.1\n"
                             "beat\n"
                             "  layer only.png\n");
    REQUIRE(result.error.empty());
    const game::Storybook& book = result.book;
    CHECK(book.music == "theme.flac");
    CHECK(book.crossfadeSeconds == 2.0F);
    REQUIRE(book.beats.size() == 2);
    const game::StorybookBeat& first = book.beats.at(0);
    CHECK(first.seconds == 7.5F);
    CHECK(first.voice == "beat1.wav");
    CHECK(first.lines.at(0) == "Every winter the moon.");
    CHECK(first.lines.at(1) == "It falls.");
    REQUIRE(first.layers.size() == 2);
    CHECK(first.layers.at(0).file == "sky.png");
    CHECK(first.layers.at(0).scaleFrom == 1.0F);
    CHECK(first.layers.at(1).driftX == doctest::Approx(-0.02));
    CHECK(first.layers.at(1).driftY == doctest::Approx(0.01));
    CHECK(first.layers.at(1).scaleFrom == doctest::Approx(1.05));
    CHECK(first.layers.at(1).scaleTo == doctest::Approx(1.1));
    // What is not named takes its default: no voice, no text, the default time.
    const game::StorybookBeat& second = book.beats.at(1);
    CHECK(second.seconds == game::DEFAULT_STORYBOOK_BEAT_SECONDS);
    CHECK(second.voice.empty());
    CHECK(second.lines.at(0).empty());
}

TEST_CASE("a manifest with a mistake is refused with the number of its line") {
    const auto errorOf = [](const char* text) { return game::parseStorybook(text).error; };
    CHECK(errorOf("").find("no beat") != std::string::npos);
    CHECK(errorOf("beat\n").find("beat 1 has no layer") != std::string::npos);
    CHECK(errorOf("layer a.png\n").find("line 1") != std::string::npos);
    CHECK(errorOf("beat\nlayer a.png\nzoom 2\n").find("line 3: unknown word") != std::string::npos);
    CHECK_FALSE(errorOf("beat\nlayer a.png\nseconds 0.2\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer a.png\nseconds six\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer a.png 0.1 0 1\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer a.png 0 0 0.5 1\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer a.png 0 0 1 1 1\n").empty());
    CHECK_FALSE(errorOf("beat\nline a\nline b\nline c\nlayer a.png\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer a\nlayer b\nlayer c\nlayer d\nlayer e\n").empty());
    // No file outside the folder of the storybook.
    CHECK_FALSE(errorOf("beat\nlayer ../settings.txt\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer sub/a.png\n").empty());
    CHECK_FALSE(errorOf("beat\nlayer C:a.png\n").empty());
    CHECK_FALSE(errorOf("music ..\\x.flac\nbeat\nlayer a.png\n").empty());
}

TEST_CASE("the storybook of the game is a valid manifest whose pictures exist") {
    const std::filesystem::path folder =
        std::filesystem::path(NIGHT_MAZE_ASSETS_DIR) / game::STORYBOOK_FOLDER;
    std::string text;
    REQUIRE(core::readTextFile(folder / game::STORYBOOK_MANIFEST_FILE, text));
    const game::StorybookResult result = game::parseStorybook(text);
    CHECK(result.error.empty());
    CHECK(result.book.beats.size() >= 5);
    CHECK(result.book.beats.size() <= 7);
    for (const game::StorybookBeat& beat : result.book.beats) {
        for (const game::StorybookLayer& layer : beat.layers) {
            CHECK(std::filesystem::exists(folder / layer.file));
        }
    }
}

TEST_CASE("a picture stays its own time, or until its spoken line has ended") {
    game::StorybookBeat beat;
    beat.seconds = 6.0F;
    CHECK(game::storybookBeatSeconds(beat, 0.0F) == 6.0F);
    CHECK(game::storybookBeatSeconds(beat, 2.0F) == 6.0F);
    const float voice = 8.0F;
    CHECK(game::storybookBeatSeconds(beat, voice) ==
          doctest::Approx(game::STORYBOOK_VOICE_DELAY_SECONDS + voice +
                          game::STORYBOOK_VOICE_TAIL_SECONDS));
}

TEST_CASE("the frames of the storybook go from black through every picture to black") {
    const std::vector<float> beats = {6.0F, 8.0F, 6.0F};
    const float fade = 1.5F;
    CHECK(game::storybookSeconds(beats) == 20.0F);

    const StorybookFrame start = game::storybookFrame(beats, fade, 0.0F);
    CHECK_FALSE(start.finished);
    CHECK(start.beat == 0);
    CHECK(start.blackOpacity == 1.0F);
    CHECK(start.progress == 0.0F);
    CHECK_FALSE(start.skippable);

    // In the middle of the first picture: no black, all of the text, no crossfade.
    const StorybookFrame middle = game::storybookFrame(beats, fade, 3.0F);
    CHECK(middle.blackOpacity == 0.0F);
    CHECK(middle.textOpacity == 1.0F);
    CHECK(middle.nextOpacity == 0.0F);
    CHECK(middle.progress == doctest::Approx(0.5));
    CHECK(middle.skippable);

    // Half way through the crossfade into the second picture: half of it is there and
    // its motion has begun.
    const StorybookFrame crossing = game::storybookFrame(beats, fade, 6.0F - 0.75F);
    CHECK(crossing.beat == 0);
    CHECK(crossing.nextOpacity == doctest::Approx(0.5));
    CHECK(crossing.nextProgress > 0.0F);
    CHECK(crossing.nextProgress < 0.1F);

    // At the cut the second picture is all there and goes on from where it was, and
    // its text has not come up yet.
    const StorybookFrame cut = game::storybookFrame(beats, fade, 6.0F);
    CHECK(cut.beat == 1);
    CHECK(cut.beatSeconds == 0.0F);
    CHECK(cut.textOpacity == 0.0F);
    CHECK(cut.progress == doctest::Approx(fade / (8.0F + fade)));
    const StorybookFrame justBefore = game::storybookFrame(beats, fade, 5.999F);
    CHECK(justBefore.nextProgress == doctest::Approx(cut.progress).epsilon(0.01));

    // The motion only ever goes forward, and the last picture fades into black.
    float last = 0.0F;
    for (int step = 0; step < 32; ++step) {
        const float progress =
            game::storybookFrame(beats, fade, 6.0F + 0.25F * static_cast<float>(step)).progress;
        CHECK(progress >= last);
        last = progress;
    }
    CHECK(game::storybookFrame(beats, fade, 19.9F).blackOpacity > 0.9F);
    CHECK(game::storybookFrame(beats, fade, 19.9F).nextOpacity == 0.0F);
    const StorybookFrame end = game::storybookFrame(beats, fade, 25.0F);
    CHECK(end.finished);
    CHECK(end.beat == 2);
}

TEST_CASE("a crossfade longer than half the shortest picture is shortened") {
    const std::vector<float> beats = {2.0F, 2.0F};
    // A crossfade of 10 s would start before the first picture; it is 1 s instead.
    CHECK(game::storybookFrame(beats, 10.0F, 0.9F).nextOpacity == 0.0F);
    CHECK(game::storybookFrame(beats, 10.0F, 1.5F).nextOpacity == doctest::Approx(0.5));
    // No crossfade at all: a cut.
    CHECK(game::storybookFrame(beats, 0.0F, 1.99F).nextOpacity == 0.0F);
}

TEST_CASE("every spoken line is started once, whatever the length of the frames") {
    const std::vector<float> beats = {6.0F, 8.0F, 6.0F};
    const std::vector<std::size_t> expected = {0, 1, 2};
    CHECK(voicesOfRun(beats, 1.0F / 60.0F) == expected);
    CHECK(voicesOfRun(beats, 1.0F / 7.0F) == expected);
    CHECK(voicesOfRun(beats, 2.5F) == expected);
    CHECK(game::storybookVoicesBetween(beats, 0.0F, game::STORYBOOK_VOICE_DELAY_SECONDS).empty());
}

TEST_CASE("a layer grows and drifts inside its picture and never past its edge") {
    const gfx::UvRect whole;
    const game::StorybookLayer still;
    const gfx::UvRect same = game::storybookLayerUv(whole, still, 0.5F);
    CHECK(same.left == 0.0F);
    CHECK(same.right == 1.0F);

    game::StorybookLayer layer{
        .file = "a.png", .driftX = 0.04F, .driftY = 0.0F, .scaleFrom = 1.1F, .scaleTo = 1.2F};
    const gfx::UvRect from = game::storybookLayerUv(whole, layer, 0.0F);
    const gfx::UvRect to = game::storybookLayerUv(whole, layer, 1.0F);
    // It grows: less of the picture fills the window at the end.
    CHECK(to.right - to.left < from.right - from.left);
    CHECK(from.right - from.left == doctest::Approx(1.0 / 1.1));
    // It moves right on the screen: the part shown moves left in the picture.
    CHECK(to.left + to.right < from.left + from.right);

    // A drift far larger than the room the scale leaves stops at the edge.
    layer.driftX = 1.0F;
    layer.driftY = -1.0F;
    for (const float progress : {0.0F, 0.25F, 1.0F}) {
        const gfx::UvRect shown = game::storybookLayerUv(whole, layer, progress);
        CHECK(shown.left >= 0.0F);
        CHECK(shown.right <= 1.0F);
        CHECK(shown.top >= 0.0F);
        CHECK(shown.bottom <= 1.0F);
    }

    // A window narrower than the picture shows its middle, and the layer stays inside.
    const gfx::UvRect narrow = gfx::coverFit(1920, 1080, 1080, 1080);
    const gfx::UvRect shown = game::storybookLayerUv(narrow, layer, 1.0F);
    CHECK(shown.left >= 0.0F);
    CHECK(shown.right <= 1.0F);
    CHECK(shown.right - shown.left == doctest::Approx((narrow.right - narrow.left) / 1.2));
}
