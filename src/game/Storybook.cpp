// Storybook: the intro as painted pictures in layers, read from a small text file.
#include "game/Storybook.hpp"

#include "game/Intro.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace game {

namespace {

// The words of the manifest.
constexpr std::string_view MUSIC_WORD = "music";
constexpr std::string_view CROSSFADE_WORD = "crossfade";
constexpr std::string_view BEAT_WORD = "beat";
constexpr std::string_view SECONDS_WORD = "seconds";
constexpr std::string_view VOICE_WORD = "voice";
constexpr std::string_view LINE_WORD = "line";
constexpr std::string_view LAYER_WORD = "layer";

// What the numbers of the manifest may be. A picture of a tenth of a second or of
// several minutes is a typing error, and so is a scale that shrinks the picture below
// the window or blows it up beyond recognition.
constexpr float MIN_BEAT_SECONDS = 1.0F;
constexpr float MAX_BEAT_SECONDS = 120.0F;
constexpr float MAX_CROSSFADE_SECONDS = 10.0F;
constexpr float MAX_DRIFT = 1.0F;
constexpr float MIN_SCALE = 1.0F;
constexpr float MAX_SCALE = 4.0F;

// The four numbers after the file of a layer.
constexpr std::size_t LAYER_NUMBER_COUNT = 4;

// Half, for the middle of a picture's time and of a rectangle.
constexpr float HALF = 0.5F;

constexpr std::string_view SPACES = " \t\r";

std::string_view trim(std::string_view text) {
    const std::size_t first = text.find_first_not_of(SPACES);
    if (first == std::string_view::npos) {
        return {};
    }
    return text.substr(first, text.find_last_not_of(SPACES) - first + 1);
}

// Splits off the first word of text and returns it; text keeps the rest, trimmed.
std::string_view nextWord(std::string_view& text) {
    const std::size_t end = text.find_first_of(SPACES);
    const std::string_view word = text.substr(0, end);
    text = end == std::string_view::npos ? std::string_view{} : trim(text.substr(end));
    return word;
}

// A number from minimum to maximum, written as a whole word. strtof and not from_chars,
// like game/StartOptions.cpp: the standard library of Apple clang cannot read a float
// with from_chars yet.
bool readNumber(std::string_view word, float minimum, float maximum, float& number) {
    const std::string text(word);
    char* end = nullptr;
    const float value = std::strtof(text.c_str(), &end);
    if (text.empty() || end != text.c_str() + text.size() || !std::isfinite(value) ||
        value < minimum || value > maximum) {
        return false;
    }
    number = value;
    return true;
}

// A file of the storybook folder: a plain name, nothing that leads out of the folder.
bool plainFileName(std::string_view name) {
    return !name.empty() && name.find_first_of("/\\:") == std::string_view::npos &&
           name.find("..") == std::string_view::npos;
}

// One line of the manifest. Returns an empty string or what is wrong with the line.
std::string readInstruction(std::string_view word, std::string_view rest, Storybook& book) {
    if (word == MUSIC_WORD) {
        if (!plainFileName(rest)) {
            return "music needs a file name in the storybook folder";
        }
        book.music = std::string(rest);
        return {};
    }
    if (word == CROSSFADE_WORD) {
        return readNumber(rest, 0.0F, MAX_CROSSFADE_SECONDS, book.crossfadeSeconds)
                   ? std::string()
                   : "crossfade needs a number of seconds from 0 to 10";
    }
    if (word == BEAT_WORD) {
        book.beats.emplace_back();
        return {};
    }
    if (word != SECONDS_WORD && word != VOICE_WORD && word != LINE_WORD && word != LAYER_WORD) {
        return "unknown word \"" + std::string(word) + "\"";
    }
    if (book.beats.empty()) {
        return std::string(word) + " comes before the first beat";
    }
    StorybookBeat& beat = book.beats.back();
    if (word == SECONDS_WORD) {
        return readNumber(rest, MIN_BEAT_SECONDS, MAX_BEAT_SECONDS, beat.seconds)
                   ? std::string()
                   : "seconds needs a number from 1 to 120";
    }
    if (word == VOICE_WORD) {
        if (!plainFileName(rest)) {
            return "voice needs a file name in the storybook folder";
        }
        beat.voice = std::string(rest);
        return {};
    }
    if (word == LINE_WORD) {
        const auto free = std::ranges::find(beat.lines, std::string());
        if (free == beat.lines.end()) {
            return "a beat has at most two lines of text";
        }
        *free = std::string(rest);
        return {};
    }
    // A layer: its file and four numbers, or its file alone (it then stands still).
    if (beat.layers.size() == MAX_STORYBOOK_LAYERS) {
        return "a beat has at most four layers";
    }
    StorybookLayer layer;
    layer.file = std::string(nextWord(rest));
    if (!plainFileName(layer.file)) {
        return "layer needs a file name in the storybook folder";
    }
    if (!rest.empty()) {
        std::array<float, LAYER_NUMBER_COUNT> numbers{};
        const std::array<float, LAYER_NUMBER_COUNT> minimum = {-MAX_DRIFT, -MAX_DRIFT, MIN_SCALE,
                                                               MIN_SCALE};
        const std::array<float, LAYER_NUMBER_COUNT> maximum = {MAX_DRIFT, MAX_DRIFT, MAX_SCALE,
                                                               MAX_SCALE};
        for (std::size_t i = 0; i < LAYER_NUMBER_COUNT; ++i) {
            if (!readNumber(nextWord(rest), minimum.at(i), maximum.at(i), numbers.at(i))) {
                return "layer needs a file, then drift x and y (-1 to 1) and scale from and "
                       "to (1 to 4)";
            }
        }
        if (!rest.empty()) {
            return "layer has more than four numbers";
        }
        layer.driftX = numbers.at(0);
        layer.driftY = numbers.at(1);
        layer.scaleFrom = numbers.at(2);
        layer.scaleTo = numbers.at(3);
    }
    beat.layers.push_back(layer);
    return {};
}

float unit(float value) {
    return std::clamp(value, 0.0F, 1.0F);
}

// The second at which every picture begins, and one more entry: the end of the last.
std::vector<float> beatStarts(const std::vector<float>& beatSeconds) {
    std::vector<float> starts(1, 0.0F);
    for (const float seconds : beatSeconds) {
        starts.push_back(starts.back() + seconds);
    }
    return starts;
}

} // namespace

StorybookResult parseStorybook(std::string_view text) {
    StorybookResult result;
    std::size_t lineNumber = 0;
    while (!text.empty()) {
        ++lineNumber;
        const std::size_t end = text.find('\n');
        std::string_view line = trim(text.substr(0, end));
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const std::string_view word = nextWord(line);
        const std::string error = readInstruction(word, line, result.book);
        if (!error.empty()) {
            result.error = "line " + std::to_string(lineNumber) + ": " + error;
            return result;
        }
    }
    if (result.book.beats.empty()) {
        result.error = "the storybook has no beat";
        return result;
    }
    for (std::size_t i = 0; i < result.book.beats.size(); ++i) {
        if (result.book.beats.at(i).layers.empty()) {
            result.error = "beat " + std::to_string(i + 1) + " has no layer";
            return result;
        }
    }
    return result;
}

float storybookBeatSeconds(const StorybookBeat& beat, float voiceSeconds) {
    if (voiceSeconds <= 0.0F) {
        return beat.seconds;
    }
    return std::max(beat.seconds,
                    STORYBOOK_VOICE_DELAY_SECONDS + voiceSeconds + STORYBOOK_VOICE_TAIL_SECONDS);
}

float storybookSeconds(const std::vector<float>& beatSeconds) {
    return beatStarts(beatSeconds).back();
}

StorybookFrame storybookFrame(const std::vector<float>& beatSeconds, float crossfadeSeconds,
                              float seconds) {
    StorybookFrame frame;
    if (beatSeconds.empty()) {
        frame.finished = true;
        return frame;
    }
    const std::vector<float> starts = beatStarts(beatSeconds);
    const float total = starts.back();
    const float shortest = *std::ranges::min_element(beatSeconds);
    const float fade = std::clamp(crossfadeSeconds, 0.0F, HALF * shortest);
    frame.finished = seconds >= total;
    const float moment = std::clamp(seconds, 0.0F, total);

    // The picture of the moment: the last one that has begun.
    const std::size_t count = beatSeconds.size();
    while (frame.beat + 1 < count && moment >= starts.at(frame.beat + 1)) {
        ++frame.beat;
    }
    frame.beatSeconds = moment - starts.at(frame.beat);

    // A picture is seen from the start of the crossfade into it (none for the first)
    // to the end of its time, and its motion runs over all of that.
    const auto progressOf = [&](std::size_t beat, float secondsIntoBeat) {
        const float fadeIn = beat == 0 ? 0.0F : fade;
        return unit((secondsIntoBeat + fadeIn) / (beatSeconds.at(beat) + fadeIn));
    };
    frame.progress = progressOf(frame.beat, frame.beatSeconds);
    const float duration = beatSeconds.at(frame.beat);
    if (frame.beat + 1 < count && fade > 0.0F && frame.beatSeconds > duration - fade) {
        frame.nextOpacity = unit((frame.beatSeconds - (duration - fade)) / fade);
        frame.nextProgress = progressOf(frame.beat + 1, frame.beatSeconds - duration);
    }

    frame.textOpacity = cardTextOpacity(frame.beatSeconds, duration);
    const float lifting = 1.0F - moment / INTRO_BLACK_FADE_SECONDS;
    const float falling = 1.0F - (total - moment) / INTRO_BLACK_FADE_SECONDS;
    frame.blackOpacity = unit(std::max(lifting, falling));
    frame.hintOpacity = skipHintOpacity(moment);
    frame.skippable = moment >= INTRO_SKIP_DELAY_SECONDS;
    return frame;
}

std::vector<std::size_t> storybookVoicesBetween(const std::vector<float>& beatSeconds, float before,
                                                float after) {
    std::vector<std::size_t> beats;
    const std::vector<float> starts = beatStarts(beatSeconds);
    for (std::size_t i = 0; i < beatSeconds.size(); ++i) {
        const float moment = starts.at(i) + STORYBOOK_VOICE_DELAY_SECONDS;
        if (moment >= before && moment < after && moment < starts.back()) {
            beats.push_back(i);
        }
    }
    return beats;
}

gfx::UvRect storybookLayerUv(const gfx::UvRect& cover, const StorybookLayer& layer,
                             float progress) {
    const float at = unit(progress);
    const float scale =
        std::max(MIN_SCALE, layer.scaleFrom + (layer.scaleTo - layer.scaleFrom) * at);
    const float coverWidth = cover.right - cover.left;
    const float coverHeight = cover.bottom - cover.top;
    const float width = coverWidth / scale;
    const float height = coverHeight / scale;
    // A layer that moves right on the screen shows a part of its picture further left,
    // and one that moves down (the picture's rows go down too) a part further up.
    const float shift = at - HALF;
    float centerX = HALF * (cover.left + cover.right) - layer.driftX * shift * coverWidth;
    float centerY = HALF * (cover.top + cover.bottom) - layer.driftY * shift * coverHeight;
    // Never past the edge of the picture: the edge pixels would be smeared there.
    centerX = std::clamp(centerX, HALF * width, 1.0F - HALF * width);
    centerY = std::clamp(centerY, HALF * height, 1.0F - HALF * height);
    return {.left = centerX - HALF * width,
            .top = centerY - HALF * height,
            .right = centerX + HALF * width,
            .bottom = centerY + HALF * height};
}

} // namespace game
