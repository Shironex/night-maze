// Tests of audio::AudioEngine: the volume of one play, the three groups, loops with their
// fades, what stopAll does to them, and the theme of the menu (a FLAC file).
//
// No test here opens a sound device, and none is heard. The engine is made without
// one (audio::AudioOutput::None) and asked for its mixed sound sample by sample
// (render), so the tests look at the very numbers a loudspeaker would be given. They
// load the real sound files of the game.
#include "audio/AudioEngine.hpp"

#include <doctest/doctest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace {

using audio::AudioEngine;
using audio::AudioOutput;
using audio::SoundFile;
using audio::SoundGroup;

// The engine without a device mixes 48 000 frames per second, in stereo.
constexpr std::size_t RATE = 48000;
constexpr std::size_t CHANNELS = 2;

// The wind of the maze is a loop of this many seconds (tools/make_sounds.py).
constexpr std::size_t WIND_SECONDS = 10;

// The theme of the menu is a loop of 72.72 seconds (tools/make_music.py): this many
// frames of the engine.
constexpr std::size_t THEME_FRAMES = 3490560;

// The directory of the sounds in the repository. NIGHT_MAZE_ASSETS_DIR is set by
// CMakeLists.txt, so the tests do not depend on the directory they are started from.
SoundFile soundFile(const char* name, SoundGroup group = SoundGroup::Effects) {
    return {.path = std::filesystem::path(NIGHT_MAZE_ASSETS_DIR) / "audio" / name, .group = group};
}

// The left channel of the next seconds of sound: every second number of what the
// engine mixes. The sounds of these tests are mono, so both channels are the same.
std::vector<float> renderLeft(AudioEngine& engine, double seconds) {
    const auto frames = static_cast<std::size_t>(seconds * static_cast<double>(RATE));
    const std::vector<float> mixed = engine.render(frames);
    std::vector<float> left(frames);
    for (std::size_t i = 0; i < frames; ++i) {
        left[i] = mixed[i * CHANNELS];
    }
    return left;
}

// One channel (0 is left, 1 is right) of sound the engine has mixed.
std::vector<float> channelOf(const std::vector<float>& mixed, std::size_t channel) {
    std::vector<float> one(mixed.size() / CHANNELS);
    for (std::size_t i = 0; i < one.size(); ++i) {
        one[i] = mixed[i * CHANNELS + channel];
    }
    return one;
}

// The largest sample of a stretch of sound, without its sign.
float peak(std::span<const float> samples) {
    float largest = 0.0F;
    for (const float sample : samples) {
        largest = std::max(largest, std::abs(sample));
    }
    return largest;
}

// The largest difference between two samples that follow each other. A sound that is
// cut, or starts at once, shows as a difference far above what the sound has anywhere
// else, and that is what a click is.
float largestStep(std::span<const float> samples) {
    float largest = 0.0F;
    for (std::size_t i = 1; i < samples.size(); ++i) {
        largest = std::max(largest, std::abs(samples[i] - samples[i - 1]));
    }
    return largest;
}

// The loudest sample of the first half second after sound 0 was played at a volume.
float peakOfPlay(AudioEngine& engine, float volume) {
    engine.play(0, volume);
    const float loudest = peak(renderLeft(engine, 0.5));
    // The rest of the sound is cut, so the next play starts in silence.
    engine.stopAll();
    renderLeft(engine, 0.1);
    return loudest;
}

} // namespace

TEST_CASE("an engine without a device mixes silence until something is played") {
    AudioEngine engine(AudioOutput::None);
    REQUIRE(engine.isAvailable());
    CHECK(engine.sampleRate() == RATE);
    CHECK(engine.channels() == CHANNELS);
    CHECK(engine.status().find("no device") != std::string::npos);
    CHECK(peak(engine.render(RATE / 10)) == 0.0F);

    // A number that was never loaded and a file that does not exist do nothing.
    const std::array<SoundFile, 2> files = {soundFile("crystal_pickup.wav"),
                                            soundFile("no_such_sound.wav")};
    engine.load(files);
    CHECK(engine.status().find("1 of 2 sounds loaded") != std::string::npos);
    engine.play(1);
    engine.play(7);
    engine.setLoop(1, true, 0.0F);
    engine.setLoop(7, true, 0.0F);
    CHECK_FALSE(engine.isLooping(1));
    CHECK(engine.loopCount() == 0);
    CHECK(peak(engine.render(RATE / 10)) == 0.0F);

    engine.play(0);
    CHECK(peak(engine.render(RATE / 10)) > 0.1F);
}

TEST_CASE("the volume of a play scales that play and does not stick to the voice") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 1> files = {soundFile("flashlight_on.wav")};
    engine.load(files);

    const float full = peakOfPlay(engine, 1.0F);
    // The file as it is: its loudest sample is at -11 dBFS (assets/audio/README.md),
    // which is 0.28. The file is mono and the engine mixes stereo: each ear gets the
    // whole sound, not a share of it.
    REQUIRE(full == doctest::Approx(0.282F).epsilon(0.03));
    CHECK(peakOfPlay(engine, 0.5F) == doctest::Approx(0.5F * full).epsilon(0.01));
    CHECK(peakOfPlay(engine, 0.25F) == doctest::Approx(0.25F * full).epsilon(0.01));
    // Every voice of the sound has now been used at a low volume. A play without
    // a number is at full volume again, on whichever voice it lands.
    for (int i = 0; i < 4; ++i) {
        engine.play(0);
        CHECK(peak(renderLeft(engine, 0.5)) == doctest::Approx(full).epsilon(0.01));
    }
    // Numbers outside of 0 to 1 are brought into the range.
    CHECK(peakOfPlay(engine, 3.0F) == doctest::Approx(full).epsilon(0.01));
    CHECK(peakOfPlay(engine, -1.0F) == 0.0F);
}

TEST_CASE("each group has a volume of its own under the master volume") {
    AudioEngine engine(AudioOutput::None);
    // The same file twice: sound 0 in the effects group, sound 1 in the ambient group.
    const std::array<SoundFile, 2> files = {soundFile("flashlight_on.wav"),
                                            soundFile("flashlight_on.wav", SoundGroup::Ambient)};
    engine.load(files);
    const auto peakOf = [&engine](std::size_t index) {
        engine.play(index);
        return peak(renderLeft(engine, 0.5));
    };
    const float full = peakOf(0);
    REQUIRE(full > 0.05F);
    CHECK(peakOf(1) == doctest::Approx(full).epsilon(0.01));

    // The ambient group at half: only its sound is quieter.
    engine.setGroupVolume(SoundGroup::Ambient, 0.5F);
    CHECK(engine.groupVolume(SoundGroup::Ambient) == 0.5F);
    CHECK(engine.groupVolume(SoundGroup::Effects) == 1.0F);
    CHECK(peakOf(0) == doctest::Approx(full).epsilon(0.01));
    CHECK(peakOf(1) == doctest::Approx(0.5F * full).epsilon(0.01));

    // The effects group silent: the ambient sound is still there.
    engine.setGroupVolume(SoundGroup::Effects, 0.0F);
    CHECK(peakOf(0) == 0.0F);
    CHECK(peakOf(1) == doctest::Approx(0.5F * full).epsilon(0.01));

    // The master volume lies over both: the three numbers multiply.
    engine.setGroupVolume(SoundGroup::Effects, 1.0F);
    engine.setMasterVolume(0.5F);
    CHECK(peakOf(0) == doctest::Approx(0.5F * full).epsilon(0.01));
    CHECK(peakOf(1) == doctest::Approx(0.25F * full).epsilon(0.01));

    // Out of range numbers are brought into the range, as for the master volume.
    engine.setGroupVolume(SoundGroup::Ambient, 7.0F);
    CHECK(engine.groupVolume(SoundGroup::Ambient) == 1.0F);
}

TEST_CASE("the music group has a volume of its own and leaves the other two alone") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 3> files = {soundFile("flashlight_on.wav"),
                                            soundFile("flashlight_on.wav", SoundGroup::Ambient),
                                            soundFile("flashlight_on.wav", SoundGroup::Music)};
    engine.load(files);
    const auto peakOf = [&engine](std::size_t index) {
        engine.play(index);
        return peak(renderLeft(engine, 0.5));
    };
    const float full = peakOf(0);
    REQUIRE(full > 0.05F);
    CHECK(engine.groupVolume(SoundGroup::Music) == 1.0F);
    CHECK(peakOf(2) == doctest::Approx(full).epsilon(0.01));

    engine.setGroupVolume(SoundGroup::Music, 0.36F);
    CHECK(engine.groupVolume(SoundGroup::Music) == 0.36F);
    CHECK(peakOf(0) == doctest::Approx(full).epsilon(0.01));
    CHECK(peakOf(1) == doctest::Approx(full).epsilon(0.01));
    CHECK(peakOf(2) == doctest::Approx(0.36F * full).epsilon(0.01));

    // The other two do not reach the music, and the master volume does.
    engine.setGroupVolume(SoundGroup::Effects, 0.0F);
    engine.setGroupVolume(SoundGroup::Ambient, 0.0F);
    CHECK(peakOf(2) == doctest::Approx(0.36F * full).epsilon(0.01));
    engine.setMasterVolume(0.0F);
    CHECK(peakOf(2) == 0.0F);
}

TEST_CASE("a loop goes on for longer than its file, and only setLoop ends it") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 2> files = {soundFile("maze_wind.wav", SoundGroup::Ambient),
                                            soundFile("intro_wind.wav", SoundGroup::Ambient)};
    engine.load(files);
    CHECK(engine.loopCount() == 0);

    engine.setLoop(0, true, 0.0F);
    CHECK(engine.isLooping(0));
    CHECK(engine.loopCount() == 1);
    // Two times the length of the file, and it still sounds. The loudest sample of one
    // whole round is the loudest the loop ever gets.
    renderLeft(engine, 2.0 * static_cast<double>(WIND_SECONDS));
    const float loud = peak(renderLeft(engine, static_cast<double>(WIND_SECONDS)));
    CHECK(loud > 0.005F);

    // stopAll ends a sound that was played and leaves the loop alone. The long wind of
    // the intro is played over the loop, and then everything is stopped at once.
    engine.play(1);
    renderLeft(engine, 4.0);
    const float both = peak(renderLeft(engine, 2.0));
    CHECK(both > 1.5F * loud);
    engine.stopAll();
    CHECK(engine.isLooping(0));
    const float after = peak(renderLeft(engine, 2.0));
    CHECK(after > 0.005F);
    CHECK(after <= loud);

    // Switched off, it is silent after its fade and stays silent.
    engine.setLoop(0, false, 0.5F);
    CHECK_FALSE(engine.isLooping(0));
    CHECK(engine.loopCount() == 0);
    renderLeft(engine, 0.6);
    CHECK(peak(renderLeft(engine, 1.0)) == 0.0F);
    // And stopAll does not bring anything back.
    engine.stopAll();
    CHECK(peak(renderLeft(engine, 0.5)) == 0.0F);

    // Switched on again, it sounds again.
    engine.setLoop(0, true, 0.5F);
    renderLeft(engine, 0.6);
    CHECK(peak(renderLeft(engine, 1.0)) > 0.005F);
}

TEST_CASE("a loop that is asked for again and again starts its fade only once") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 1> files = {soundFile("maze_wind.wav", SoundGroup::Ambient)};
    engine.load(files);

    // The game says in every frame what it wants. A fade that started again with every
    // call would never get anywhere: after two seconds of calls the fade of one second
    // has to be over, and the loop as loud as one that was never faded.
    for (int frame = 0; frame < 120; ++frame) {
        engine.setLoop(0, true, 1.0F);
        engine.render(RATE / 60);
    }
    const float faded = peak(renderLeft(engine, 2.0));

    AudioEngine plain(AudioOutput::None);
    plain.load(files);
    plain.setLoop(0, true, 0.0F);
    plain.render(2 * RATE);
    CHECK(faded == doctest::Approx(peak(renderLeft(plain, 2.0))).epsilon(0.001));
}

TEST_CASE("a loop fades in and out without a click, also when the fade is turned round") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 1> files = {soundFile("maze_wind.wav", SoundGroup::Ambient)};
    engine.load(files);

    // What is ordinary for this sound: the largest step between two samples in one
    // whole round of it at full loudness (with a hundredth on top for the rounding of
    // the fade). A fade only makes the sound quieter, so no step of a clean fade can be
    // larger than that.
    engine.setLoop(0, true, 0.0F);
    const float ordinary =
        1.01F * largestStep(renderLeft(engine, static_cast<double>(WIND_SECONDS)));
    REQUIRE(ordinary > 0.0F);
    engine.setLoop(0, false, 0.0F);
    renderLeft(engine, 0.1);

    // Fading in from silence: the first samples are almost nothing, and no step on the
    // way up is larger than the sound has by itself.
    engine.setLoop(0, true, 0.5F);
    std::vector<float> sound = renderLeft(engine, 1.0);
    CHECK(std::abs(sound.front()) < 0.01F * ordinary);
    CHECK(largestStep(sound) <= ordinary);
    CHECK(peak(std::span(sound).last(RATE / 4)) > 10.0F * peak(std::span(sound).first(RATE / 100)));

    // Fading out: the same on the way down, and the last samples are silence.
    engine.setLoop(0, false, 0.5F);
    sound = renderLeft(engine, 1.0);
    CHECK(largestStep(sound) <= ordinary);
    CHECK(peak(std::span(sound).last(RATE / 4)) == 0.0F);

    // A fade out that is turned round half way (the pause menu opened and closed at
    // once): the sound goes on from the loudness it had, without a jump and without
    // a moment of silence.
    engine.setLoop(0, true, 0.0F);
    renderLeft(engine, 0.5);
    engine.setLoop(0, false, 1.0F);
    const std::vector<float> down = renderLeft(engine, 0.5);
    engine.setLoop(0, true, 1.0F);
    const std::vector<float> up = renderLeft(engine, 2.0);
    CHECK(std::abs(up.front() - down.back()) <= ordinary);
    CHECK(largestStep(up) <= ordinary);
    CHECK(peak(std::span(up).first(RATE / 10)) > 0.0F);
    // The stop that the fade out had asked for does not come: it still sounds later.
    CHECK(peak(renderLeft(engine, 1.0)) > 0.005F);
}

TEST_CASE("the wind of the maze has no tick where its file starts again") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 1> files = {soundFile("maze_wind.wav", SoundGroup::Ambient)};
    engine.load(files);
    engine.setLoop(0, true, 0.0F);

    // 25 seconds: the file starts again after 10 and after 20. The engine mixes at
    // another sample rate than the file has, and the conversion starts from silence,
    // which is why the file has to begin where its wave crosses 0. A copy of the file
    // that begins at its loudest sample fails this test with a step ten times the
    // ordinary one.
    const std::vector<float> sound = renderLeft(engine, 25.0);
    constexpr std::size_t AROUND = 64;
    for (const std::size_t round : {std::size_t{1}, std::size_t{2}}) {
        const std::size_t seam = round * WIND_SECONDS * RATE;
        // The second before the seam says what is ordinary there. The few samples
        // around the seam must not hold a larger step than that whole second.
        const float before = largestStep(std::span(sound).subspan(seam - RATE, RATE - AROUND));
        const float across = largestStep(std::span(sound).subspan(seam - AROUND, 2 * AROUND));
        CHECK(before > 0.0F);
        CHECK(across <= before);
        // And the loudness does not dip or jump there: the tenth of a second after the
        // seam is as loud as the one before it, within a factor of two.
        const float left = peak(std::span(sound).subspan(seam - RATE / 10, RATE / 10));
        const float right = peak(std::span(sound).subspan(seam, RATE / 10));
        CHECK(right > 0.5F * left);
        CHECK(right < 2.0F * left);
    }
}

TEST_CASE("the theme of the menu is a FLAC file that loads and goes round without a tick") {
    AudioEngine engine(AudioOutput::None);
    const std::array<SoundFile, 1> files = {soundFile("menu_theme.flac", SoundGroup::Music)};
    engine.load(files);
    // A build of miniaudio without its FLAC decoder fails here.
    REQUIRE(engine.status().find("1 of 1 sounds loaded") != std::string::npos);

    // One round and a second of the next one, in both channels: they are different
    // sounds here.
    engine.setLoop(0, true, 0.0F);
    const std::vector<float> mixed = engine.render(THEME_FRAMES + RATE);
    const std::vector<float> left = channelOf(mixed, 0);
    const std::vector<float> right = channelOf(mixed, 1);
    // The loudest sample of the file is at -10 dBFS (tools/make_music.py), which is
    // 0.316. The conversion to the rate of the engine moves that a little.
    CHECK(peak(mixed) == doctest::Approx(0.316F).epsilon(0.05));
    // The strings sit left and right, so the two channels are not the same sound.
    float apart = 0.0F;
    for (std::size_t i = 0; i < left.size(); ++i) {
        apart = std::max(apart, std::abs(left[i] - right[i]));
    }
    CHECK(apart > 0.02F);
    // It never falls silent for long: every five seconds of it hold sound.
    for (std::size_t start = 0; start + 5 * RATE <= left.size(); start += 5 * RATE) {
        CHECK(peak(std::span(left).subspan(start, 5 * RATE)) > 0.002F);
    }

    // The seam, asked the way the wind of the maze is asked: the few samples around it
    // hold no larger step than the whole second before it.
    constexpr std::size_t AROUND = 64;
    for (const std::vector<float>* sound : {&left, &right}) {
        const std::span<const float> samples(*sound);
        const float before = largestStep(samples.subspan(THEME_FRAMES - RATE, RATE - AROUND));
        const float across = largestStep(samples.subspan(THEME_FRAMES - AROUND, 2 * AROUND));
        CHECK(before > 0.0F);
        CHECK(across <= before);
        // The piece is quiet at its seam, a moment before the first note: what follows
        // is neither silence nor louder than the echo before it.
        const float last = peak(samples.subspan(THEME_FRAMES - RATE / 10, RATE / 10));
        const float first = peak(samples.subspan(THEME_FRAMES, RATE / 10));
        CHECK(first > 0.5F * last);
        CHECK(first < 2.0F * last);
    }
}

TEST_CASE("a loop that is switched off and on again goes on from where it stopped") {
    const std::array<SoundFile, 1> files = {soundFile("maze_wind.wav", SoundGroup::Ambient)};
    // One engine plays the loop without a break.
    AudioEngine plain(AudioOutput::None);
    plain.load(files);
    plain.setLoop(0, true, 0.0F);
    const std::vector<float> whole = renderLeft(plain, 6.0);

    // The other one is switched off after two seconds (a night starts, for the theme of
    // the menu), with a fade of one second, stays off, and is switched on again (the
    // menu is back).
    AudioEngine engine(AudioOutput::None);
    engine.load(files);
    engine.setLoop(0, true, 0.0F);
    renderLeft(engine, 2.0);
    engine.setLoop(0, false, 1.0F);
    renderLeft(engine, 1.5);
    CHECK(peak(renderLeft(engine, 2.0)) == 0.0F);
    engine.setLoop(0, true, 0.0F);
    const std::vector<float> resumed = renderLeft(engine, 0.5);
    REQUIRE(peak(resumed) > 0.001F);

    // What comes then is the loop from its fourth second on, not from its first: it
    // went on for the second of its fade and then stood still. The place is looked for
    // in the unbroken sound, a hundredth of a second to either side. The first tenth of
    // a second is left out: the engine brings a voice that starts up to its loudness
    // over a few milliseconds, also when no fade was asked for.
    const auto differenceAt = [&](std::size_t start) {
        float difference = 0.0F;
        for (std::size_t i = RATE / 10; i < resumed.size(); ++i) {
            difference = std::max(difference, std::abs(resumed[i] - whole[start + i]));
        }
        return difference;
    };
    float best = 1.0F;
    std::size_t place = 0;
    for (std::size_t start = 3 * RATE - RATE / 100; start <= 3 * RATE + RATE / 100; ++start) {
        const float difference = differenceAt(start);
        if (difference < best) {
            best = difference;
            place = start;
        }
    }
    CAPTURE(place);
    CHECK(best < 0.01F * peak(resumed));
    CHECK(differenceAt(0) > 0.5F * peak(resumed));
}
