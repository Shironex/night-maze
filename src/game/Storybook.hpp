// Storybook: the intro as painted pictures in layers, read from a small text file.
#pragma once

#include "gfx/CoverFit.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace game {

// Plain data and pure functions without a window, without OpenGL and without a sound
// card, like the rest of the game_logic library, so tests can check every rule. The
// application (NightMazeApp) loads the pictures and the sounds the file names, counts
// the seconds and asks what to show (storybookFrame). StorybookRenderer draws it.
//
// The storybook is the other way to tell the intro (game/Intro.hpp tells it live with
// the menu camera): five to seven painted pictures, each cut into a few layers that
// drift and grow slowly, one over the other, with a spoken line and a line of text. The
// game plays it when assets/intro/storybook.txt can be read, unless the switch
// --intro-style classic asks for the live intro. docs/story/intro-storyboard.md says
// what the pictures show and how to make the files.

/// The folder of the storybook, relative to the assets directory, and its manifest in
/// that folder. Every file the manifest names lies in the same folder.
constexpr const char* STORYBOOK_FOLDER = "intro";
constexpr const char* STORYBOOK_MANIFEST_FILE = "storybook.txt";

/// The most layers one picture can have, and the most lines of text under it (the card
/// of the intro shows two, game/Intro.hpp).
constexpr std::size_t MAX_STORYBOOK_LAYERS = 4;
constexpr std::size_t STORYBOOK_LINE_COUNT = 2;

/// One layer of a picture: a PNG file (with alpha for every layer but the first) and
/// how it moves over the time of its picture. The drift is how far the layer travels,
/// as a share of the window width and height (0.05 is five percent), centred on the
/// middle of the picture's time. The scale is how much of the picture is enlarged at
/// the start and at the end: 1 shows all of it, 1.1 shows the middle 91 percent.
/// A layer near the eye drifts and grows more than one far away, and that difference is
/// the depth. A layer is never moved past its own edge: a drift larger than the
/// enlargement leaves room for is stopped there (storybookLayerUv).
struct StorybookLayer {
    std::string file;
    float driftX = 0.0F;
    float driftY = 0.0F;
    float scaleFrom = 1.0F;
    float scaleTo = 1.0F;
};

/// The default time of a picture, in seconds, when the manifest names none.
constexpr float DEFAULT_STORYBOOK_BEAT_SECONDS = 6.0F;

/// One picture of the story: its layers from the back to the front, its text, its
/// spoken line and how long it stays.
struct StorybookBeat {
    /// How long the picture stays at least, in seconds. With a spoken line it stays
    /// until the line has ended and a moment more (storybookBeatSeconds).
    float seconds = DEFAULT_STORYBOOK_BEAT_SECONDS;
    /// The spoken line, a WAV or FLAC file. Empty: the picture is silent.
    std::string voice;
    /// The two lines of text in the lower third. The second may be empty.
    std::array<std::string, STORYBOOK_LINE_COUNT> lines;
    /// The layers, the back one first. The back one should cover the whole picture
    /// without holes: the crossfade lays the next picture over it.
    std::vector<StorybookLayer> layers;
};

/// The default length of the crossfade between two pictures, in seconds.
constexpr float DEFAULT_STORYBOOK_CROSSFADE_SECONDS = 1.5F;

/// The whole storybook.
struct Storybook {
    /// The music under the whole intro, a WAV or FLAC file. Empty: no music.
    std::string music;
    /// How long one picture takes to fade into the next, in seconds.
    float crossfadeSeconds = DEFAULT_STORYBOOK_CROSSFADE_SECONDS;
    std::vector<StorybookBeat> beats;
};

/// What parseStorybook found.
struct StorybookResult {
    /// Only meaningful when error is empty.
    Storybook book;
    /// Empty when the whole file was understood. Otherwise one line for the log that
    /// names the line of the file.
    std::string error;
};

/// Reads the text of a manifest. One instruction per line, a word and its values;
/// empty lines and lines that start with # are skipped:
///
///     music <file>                 the music under the whole intro (optional)
///     crossfade <seconds>          the crossfade between two pictures (optional)
///     beat                         a new picture; the lines below belong to it
///     seconds <seconds>            how long it stays at least
///     voice <file>                 its spoken line (optional)
///     line <text>                  a line of text, at most two per picture
///     layer <file> [<drift x> <drift y> <scale from> <scale to>]
///                                  a layer, the back one first, at most four
///
/// A file is a plain name in the folder of the manifest: no folders, no "..". An
/// unknown word, a missing or wrong value, a picture without layers and a file without
/// pictures are errors.
StorybookResult parseStorybook(std::string_view text);

/// When the spoken line of a picture begins, in seconds after the picture came up, and
/// how long the picture stays after the line has ended.
constexpr float STORYBOOK_VOICE_DELAY_SECONDS = 0.6F;
constexpr float STORYBOOK_VOICE_TAIL_SECONDS = 1.6F;

/// How long a picture stays, in seconds: its own time, or longer when its spoken line
/// needs it. voiceSeconds is the length of that line, 0 for none (also when the sound
/// could not be loaded or the computer has no sound).
float storybookBeatSeconds(const StorybookBeat& beat, float voiceSeconds);

/// The sum of the times of the pictures.
float storybookSeconds(const std::vector<float>& beatSeconds);

/// What the storybook shows at one moment.
struct StorybookFrame {
    /// True at or past the end. The other fields then describe the last moment.
    bool finished = false;
    /// The picture of this moment and how far through its motion it is, 0 to 1.
    std::size_t beat = 0;
    float beatSeconds = 0.0F;
    float progress = 0.0F;
    /// The next picture, laid over this one during the crossfade: how much of it is
    /// there (0 is none) and how far through its motion it is.
    float nextOpacity = 0.0F;
    float nextProgress = 0.0F;
    /// The text, the black over the pictures and the skip hint, as in the classic intro
    /// (game::IntroFrame).
    float textOpacity = 0.0F;
    float blackOpacity = 1.0F;
    float hintOpacity = 0.0F;
    bool skippable = false;
};

/// The storybook seconds after it began. beatSeconds are the times of the pictures
/// (storybookBeatSeconds), crossfadeSeconds the time of a crossfade; a crossfade longer
/// than half a picture is shortened to that. A negative moment counts as 0.
StorybookFrame storybookFrame(const std::vector<float>& beatSeconds, float crossfadeSeconds,
                              float seconds);

/// The pictures whose spoken lines begin in the stretch from before (included) to after
/// (not included), seconds since the storybook began. Called once per frame with the
/// clock before and after the frame, it names every line exactly once.
std::vector<std::size_t> storybookVoicesBetween(const std::vector<float>& beatSeconds, float before,
                                                float after);

/// The part of a layer's picture the window shows at progress (0 to 1): the cover fit
/// of the picture to the window (gfx::coverFit), enlarged by the layer's scale and moved
/// by its drift. The result always lies inside the picture.
gfx::UvRect storybookLayerUv(const gfx::UvRect& cover, const StorybookLayer& layer, float progress);

} // namespace game
