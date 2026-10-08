// Settings: what the player changes on the settings screen, and the text of the file
// they are kept in.
#include "game/Settings.hpp"

#include "game/Interactables.hpp"
#include "game/StartOptions.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace game {

namespace {

// The two values of a switch, as the file writes them.
constexpr std::string_view ON_VALUE = "on";
constexpr std::string_view OFF_VALUE = "off";

// A line that starts with this character is a comment.
constexpr char COMMENT_SIGN = '#';
// Between the name and the value of a setting.
constexpr char ASSIGN_SIGN = '=';
// Between the width and the height of a window size.
constexpr char SIZE_SIGN = 'x';

// The three bytes some editors (Notepad) put in front of a UTF-8 text: the byte order
// mark. They are not part of the first name.
constexpr std::string_view BYTE_ORDER_MARK = "\xEF\xBB\xBF";

// A number in the file is never longer than this many characters. A longer text is not
// read: it keeps the sum below far away from the limits of its type.
constexpr std::size_t MAX_NUMBER_LENGTH = 12;

constexpr int DECIMAL_BASE = 10;

// The text without the spaces, tabs and line end characters at its two ends.
std::string_view trimmed(std::string_view text) {
    constexpr std::string_view BLANKS = " \t\r\n";
    const std::size_t first = text.find_first_not_of(BLANKS);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(BLANKS);
    return text.substr(first, last - first + 1);
}

// Reads a number written with digits and at most one point: "60", "4.5", ".5". No sign
// and no exponent. False for anything else.
//
// Written by hand on purpose. The functions of the C library (strtof) read "4,5" in
// place of "4.5" when the system is set to a language that writes a comma, so a file
// would mean one thing on one computer and another thing on the next.
bool parseNumber(std::string_view text, float& number) {
    if (text.empty() || text.size() > MAX_NUMBER_LENGTH) {
        return false;
    }
    double value = 0.0;
    // The worth of the next digit after the point: a tenth, a hundredth and so on.
    double place = 1.0;
    bool afterPoint = false;
    bool anyDigit = false;
    for (const char character : text) {
        if (character == '.') {
            if (afterPoint) {
                return false;
            }
            afterPoint = true;
            continue;
        }
        if (character < '0' || character > '9') {
            return false;
        }
        anyDigit = true;
        const double digit = character - '0';
        if (afterPoint) {
            place /= DECIMAL_BASE;
            value += digit * place;
        } else {
            value = value * DECIMAL_BASE + digit;
        }
    }
    if (!anyDigit) {
        return false;
    }
    number = static_cast<float>(value);
    return true;
}

// Reads a window size, "1280x720": two whole numbers with an x between them. Each is
// brought into the limits of a window size. False for anything else.
bool parseWindowSize(std::string_view text, WindowSize& size) {
    const std::size_t sign = text.find(SIZE_SIGN);
    if (sign == std::string_view::npos) {
        return false;
    }
    float width = 0.0F;
    float height = 0.0F;
    if (!parseNumber(trimmed(text.substr(0, sign)), width) ||
        !parseNumber(trimmed(text.substr(sign + 1)), height)) {
        return false;
    }
    size.width = std::clamp(static_cast<int>(std::lround(width)), MIN_WINDOW_SIZE.width,
                            MAX_WINDOW_SIZE.width);
    size.height = std::clamp(static_cast<int>(std::lround(height)), MIN_WINDOW_SIZE.height,
                             MAX_WINDOW_SIZE.height);
    return true;
}

// A number with one digit after the point, "4.0". Built from whole numbers, so the
// point is a point on every system.
std::string oneDecimalText(float number) {
    const long tenths = std::lround(number * static_cast<float>(DECIMAL_BASE));
    return std::to_string(tenths / DECIMAL_BASE) + "." + std::to_string(tenths % DECIMAL_BASE);
}

// One line of the file.
std::string settingLine(std::string_view name, const std::string& value) {
    return std::string(name) + " = " + value + "\n";
}

} // namespace

bool applySetting(GameSettings& settings, std::string_view name, std::string_view value) {
    if (name == MOUSE_SENSITIVITY_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        settings.mouseSensitivity =
            std::clamp(number, MIN_MOUSE_SENSITIVITY, MAX_MOUSE_SENSITIVITY);
        return true;
    }
    if (name == FIELD_OF_VIEW_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        settings.fieldOfViewDegrees =
            std::clamp(number, MIN_FIELD_OF_VIEW_DEGREES, MAX_FIELD_OF_VIEW_DEGREES);
        return true;
    }
    if (name == FULLSCREEN_SETTING) {
        if (value != ON_VALUE && value != OFF_VALUE) {
            return false;
        }
        settings.fullscreen = value == ON_VALUE;
        return true;
    }
    if (name == WINDOW_SIZE_SETTING) {
        return parseWindowSize(value, settings.windowSize);
    }
    if (name == DIFFICULTY_SETTING) {
        return difficultyFromKey(value, settings.difficulty);
    }
    if (name == MASTER_VOLUME_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        // Whole numbers only: the screen and the file show no decimal places, so a value
        // with some would not come back the same after it was written and read.
        settings.masterVolume =
            std::clamp(std::round(number), MIN_MASTER_VOLUME, MAX_MASTER_VOLUME);
        return true;
    }
    // The two volumes under the master volume: read exactly like it.
    if (name == EFFECTS_VOLUME_SETTING || name == AMBIENT_VOLUME_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        float& volume =
            name == EFFECTS_VOLUME_SETTING ? settings.effectsVolume : settings.ambientVolume;
        volume = std::clamp(std::round(number), MIN_MASTER_VOLUME, MAX_MASTER_VOLUME);
        return true;
    }
    if (name == STORY_LINE_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        // A number past the end of the table (a hand written file) goes round to the
        // start, the way the counter does when a maze is finished.
        // fmod and not %, because a 12 digit number does not fit a whole number.
        settings.nextStoryLine =
            static_cast<int>(std::fmod(std::round(number), static_cast<float>(flavourLineCount())));
        return true;
    }
    if (name == OLD_STORY_LINE_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        // The same, in the shorter table the number was counted in, and then to the
        // place that line has today.
        const auto oldLine = static_cast<int>(
            std::fmod(std::round(number), static_cast<float>(oldStoryLineCount())));
        settings.nextStoryLine = storyLineFromOldTable(oldLine);
        return true;
    }
    if (name == CALM_NIGHT_SETTING) {
        if (value != ON_VALUE && value != OFF_VALUE) {
            return false;
        }
        settings.calmNight = value == ON_VALUE;
        return true;
    }
    if (name == INTRO_SEEN_SETTING) {
        if (value != ON_VALUE && value != OFF_VALUE) {
            return false;
        }
        settings.introSeen = value == ON_VALUE;
        return true;
    }
    if (name == CAMPAIGN_NIGHT_SETTING) {
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        // Clamped as a float first: a 12 digit number does not fit a whole number.
        settings.campaignNight = static_cast<int>(
            std::clamp(std::round(number), 1.0F, static_cast<float>(CAMPAIGN_FINISHED)));
        return true;
    }
    if (name == CAMPAIGN_SEED_SETTING) {
        // Digits only and exact: parseNumber reads a float, which cannot hold every seed.
        return parseSeed(value, settings.campaignSeed);
    }
    if (name.starts_with(CAMPAIGN_BEST_SETTING_PREFIX)) {
        // What follows the prefix is the number of the night: one digit, 1 to 5.
        const std::string_view digits = name.substr(CAMPAIGN_BEST_SETTING_PREFIX.size());
        if (digits.size() != 1 || digits.front() < '1' ||
            digits.front() > '0' + CAMPAIGN_NIGHT_COUNT) {
            return false;
        }
        float number = 0.0F;
        if (!parseNumber(value, number)) {
            return false;
        }
        settings.campaignBestSeconds.at(static_cast<std::size_t>(digits.front() - '1')) =
            static_cast<int>(std::clamp(std::round(number), static_cast<float>(NO_BEST_TIME),
                                        static_cast<float>(MAX_BEST_SECONDS)));
        return true;
    }
    if (name.starts_with(KEY_SETTING_PREFIX)) {
        // False for a name that is no action, for a text that is no key and for a key
        // with a fixed meaning: the action keeps the key it has.
        return applyKeySetting(settings.keys, name, value);
    }
    return false;
}

GameSettings parseSettings(std::string_view text) {
    GameSettings settings;
    if (text.starts_with(BYTE_ORDER_MARK)) {
        text.remove_prefix(BYTE_ORDER_MARK.size());
    }

    // The old story line counter is not applied where it stands: it counts only when the
    // file has no new one, so it is kept until every line was read.
    std::string_view oldStoryLine;
    bool hasOldStoryLine = false;
    bool hasStoryLine = false;

    // Line after line: the text up to the next line end, then the rest.
    while (!text.empty()) {
        const std::size_t lineEnd = text.find('\n');
        const std::string_view line = trimmed(text.substr(0, lineEnd));
        // Without a line end this was the last line: nothing is left.
        text = lineEnd == std::string_view::npos ? std::string_view{} : text.substr(lineEnd + 1);

        if (line.empty() || line.front() == COMMENT_SIGN) {
            continue;
        }
        const std::size_t sign = line.find(ASSIGN_SIGN);
        if (sign == std::string_view::npos) {
            continue;
        }
        const std::string_view name = trimmed(line.substr(0, sign));
        const std::string_view value = trimmed(line.substr(sign + 1));
        if (name == OLD_STORY_LINE_SETTING) {
            oldStoryLine = value;
            hasOldStoryLine = true;
            continue;
        }
        // A line that cannot be read changes nothing: applySetting returns false.
        const bool applied = applySetting(settings, name, value);
        hasStoryLine = hasStoryLine || (applied && name == STORY_LINE_SETTING);
    }
    if (hasOldStoryLine && !hasStoryLine) {
        applySetting(settings, OLD_STORY_LINE_SETTING, oldStoryLine);
    }
    // A night that is not won has no best time, whatever a hand edited file says: a time
    // that stayed would be the one a real win has to beat.
    for (int night = 1; night <= CAMPAIGN_NIGHT_COUNT; ++night) {
        if (nightStatus(settings.campaignNight, night) != NightStatus::Finished) {
            settings.campaignBestSeconds.at(static_cast<std::size_t>(night - 1)) = NO_BEST_TIME;
        }
    }
    return settings;
}

std::string formatSettings(const GameSettings& settings) {
    std::string text = "# Night Maze settings. One \"name = value\" per line, a line that starts "
                       "with # is a comment.\n";
    text +=
        settingLine(MOUSE_SENSITIVITY_SETTING, mouseSensitivityLabel(settings.mouseSensitivity));
    text += settingLine(FIELD_OF_VIEW_SETTING,
                        std::to_string(std::lround(settings.fieldOfViewDegrees)));
    text +=
        settingLine(FULLSCREEN_SETTING, std::string(settings.fullscreen ? ON_VALUE : OFF_VALUE));
    text += settingLine(WINDOW_SIZE_SETTING, windowSizeValue(settings.windowSize));
    text += settingLine(DIFFICULTY_SETTING, difficultyLevel(settings.difficulty).key);
    text += settingLine(MASTER_VOLUME_SETTING, masterVolumeLabel(settings.masterVolume));
    text += settingLine(EFFECTS_VOLUME_SETTING, masterVolumeLabel(settings.effectsVolume));
    text += settingLine(AMBIENT_VOLUME_SETTING, masterVolumeLabel(settings.ambientVolume));
    text += settingLine(STORY_LINE_SETTING, std::to_string(settings.nextStoryLine));
    text += settingLine(CALM_NIGHT_SETTING, std::string(settings.calmNight ? ON_VALUE : OFF_VALUE));
    text += settingLine(INTRO_SEEN_SETTING, std::string(settings.introSeen ? ON_VALUE : OFF_VALUE));
    text += settingLine(CAMPAIGN_NIGHT_SETTING, std::to_string(settings.campaignNight));
    // The seed and the best times only once they exist.
    if (settings.campaignSeed != NO_CAMPAIGN_SEED) {
        text += settingLine(CAMPAIGN_SEED_SETTING, std::to_string(settings.campaignSeed));
    }
    for (int night = 1; night <= CAMPAIGN_NIGHT_COUNT; ++night) {
        const int best = settings.campaignBestSeconds.at(static_cast<std::size_t>(night - 1));
        if (best != NO_BEST_TIME) {
            text += settingLine(campaignBestSetting(night), std::to_string(best));
        }
    }
    // The keys, by their names: one line per action, in the order of the settings screen.
    for (const KeyActionInfo& info : keyActions()) {
        text += settingLine(info.settingName, boundKeyName(settings.keys, info.action));
    }
    return text;
}

std::string campaignBestSetting(int night) {
    return std::string(CAMPAIGN_BEST_SETTING_PREFIX) + std::to_string(night);
}

float mouseDegreesPerUnit(float sensitivity) {
    return sensitivity * MOUSE_DEGREES_PER_SENSITIVITY;
}

std::string mouseSensitivityLabel(float sensitivity) {
    return oneDecimalText(sensitivity);
}

std::string fieldOfViewLabel(float degrees) {
    return std::to_string(std::lround(degrees)) + " deg";
}

std::string masterVolumeLabel(float volume) {
    return std::to_string(std::lround(volume));
}

float masterVolumeGain(float volume) {
    const float share = std::clamp(volume / MAX_MASTER_VOLUME, 0.0F, 1.0F);
    return share * share;
}

std::string windowSizeValue(WindowSize size) {
    return std::to_string(size.width) + SIZE_SIGN + std::to_string(size.height);
}

std::string windowSizeLabel(WindowSize size) {
    return std::to_string(size.width) + " x " + std::to_string(size.height);
}

std::vector<WindowSize> windowSizeChoices(WindowSize desktop, WindowSize current) {
    std::vector<WindowSize> choices;
    for (const WindowSize& size : WINDOW_SIZES) {
        const bool fits = size.width <= desktop.width && size.height <= desktop.height;
        if (fits || size == DEFAULT_WINDOW_SIZE || size == current) {
            choices.push_back(size);
        }
    }
    if (std::ranges::find(choices, current) == choices.end()) {
        // A size of its own: it goes in front of the first larger one. Wider counts as
        // larger, and of two sizes of the same width the higher one.
        const auto larger = std::ranges::find_if(choices, [current](WindowSize size) {
            return size.width > current.width ||
                   (size.width == current.width && size.height > current.height);
        });
        choices.insert(larger, current);
    }
    return choices;
}

WindowSize steppedWindowSize(const std::vector<WindowSize>& choices, WindowSize current, int step) {
    const auto found = std::ranges::find(choices, current);
    if (found == choices.end()) {
        return current;
    }
    // The place in the list as a whole number, moved by the step and kept in the list.
    const auto last = static_cast<int>(choices.size()) - 1;
    const int place = std::clamp(static_cast<int>(found - choices.begin()) + step, 0, last);
    return choices[static_cast<std::size_t>(place)];
}

} // namespace game
