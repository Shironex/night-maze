// MenuBackground: what is shown behind the main menu, and the rule that chooses it.
#include "game/MenuBackground.hpp"

namespace game {

namespace {

// The names of the three backgrounds, as they are typed and logged.
constexpr const char* VIDEO_NAME = "video";
constexpr const char* STILL_NAME = "still";
constexpr const char* LIVE_SCENE_NAME = "scene";

// The reasons of a choice that do not depend on an error text.
constexpr const char* ASKED_FOR_REASON = "asked for on the command line";
constexpr const char* VIDEO_PLAYS_REASON = "the video plays";
constexpr const char* NO_STILL_REASON = "the still image cannot be loaded either";

// Why the video is not shown: the error of the player, or a general text without one.
std::string videoFailedReason(std::string_view videoError) {
    if (videoError.empty()) {
        return "the video cannot be played";
    }
    return "the video cannot be played: " + std::string(videoError);
}

} // namespace

MenuBackgroundChoice chooseMenuBackground(MenuBackground wanted, bool videoPlays, bool stillLoaded,
                                          std::string_view videoError) {
    if (wanted == MenuBackground::LiveScene) {
        return {.background = MenuBackground::LiveScene, .reason = ASKED_FOR_REASON};
    }
    if (wanted == MenuBackground::Video && videoPlays) {
        return {.background = MenuBackground::Video, .reason = VIDEO_PLAYS_REASON};
    }

    // From here on the video is not shown: it was not asked for, or it failed.
    const std::string whyNoVideo =
        wanted == MenuBackground::Still ? ASKED_FOR_REASON : videoFailedReason(videoError);
    if (stillLoaded) {
        return {.background = MenuBackground::Still, .reason = whyNoVideo};
    }
    return {.background = MenuBackground::LiveScene,
            .reason = whyNoVideo + ", and " + NO_STILL_REASON};
}

bool coversWindow(MenuBackground background) {
    return background != MenuBackground::LiveScene;
}

const char* menuBackgroundName(MenuBackground background) {
    switch (background) {
    case MenuBackground::Video:
        return VIDEO_NAME;
    case MenuBackground::Still:
        return STILL_NAME;
    case MenuBackground::LiveScene:
        return LIVE_SCENE_NAME;
    }
    return LIVE_SCENE_NAME;
}

bool parseMenuBackground(std::string_view name, MenuBackground& background) {
    if (name == VIDEO_NAME) {
        background = MenuBackground::Video;
    } else if (name == STILL_NAME) {
        background = MenuBackground::Still;
    } else if (name == LIVE_SCENE_NAME) {
        background = MenuBackground::LiveScene;
    } else {
        return false;
    }
    return true;
}

} // namespace game
