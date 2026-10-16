// RoundMusic: the quiet music under a round, the layer of the chase and the sting of a
// won night, and the rules that decide when each is heard.
#include "game/RoundMusic.hpp"

#include <algorithm>

namespace game {

namespace {

// The part of both loops that is the same: a round that runs and can be heard.
bool roundHeard(const RoundMusicRequest& request, const Round& round) {
    return updatesRound(request.mode) && request.windowFocused && !request.menuCamera &&
           !request.catching && round.state == RoundState::Playing;
}

// A switch over GameMode that leaves a screen out is an error here, as in SoundCues.cpp:
// whether a new screen hears the sting is a decision.
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(error : 4062)
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic error "-Wswitch"
#endif

bool nightEndScreenOf(GameMode mode) {
    switch (mode) {
    case GameMode::RoundEnd:
    case GameMode::VillageBeat:
    case GameMode::EndingCard:
        return true;
    case GameMode::MainMenu:
    case GameMode::Playing:
    case GameMode::Paused:
    case GameMode::Quitting:
    case GameMode::SettingsFromMenu:
    case GameMode::SettingsFromPause:
    case GameMode::Intro:
    case GameMode::FreePlay:
    case GameMode::Nights:
    case GameMode::NewCampaign:
    case GameMode::NightCard:
    case GameMode::CampaignIntro:
    case GameMode::LedgerFromMenu:
    case GameMode::LedgerFromPause:
        return false;
    }
    // A number that is no screen at all.
    return false;
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#else
#pragma GCC diagnostic pop
#endif

} // namespace

bool shadeHunts(const Shade& shade) {
    return shade.present && shade.hunt != ShadeHunt::Wandering && shade.quietLeft <= 0.0F &&
           shade.graceLeft <= 0.0F;
}

bool mazeAmbientPlays(const RoundMusicRequest& request, const Round& round) {
    return roundHeard(request, round) && round.elapsedSeconds >= MAZE_AMBIENT_DELAY_SECONDS;
}

bool shadeChasePlays(const RoundMusicRequest& request, const Round& round) {
    return roundHeard(request, round) && shadeHunts(round.shade);
}

float advanceAmbientDuck(float volume, bool ducked, float frameSeconds) {
    const float target = ducked ? MAZE_AMBIENT_DUCKED_VOLUME : 1.0F;
    const float step = (1.0F - MAZE_AMBIENT_DUCKED_VOLUME) * std::max(frameSeconds, 0.0F) /
                       MAZE_AMBIENT_DUCK_SECONDS;
    return volume < target ? std::min(volume + step, target) : std::max(volume - step, target);
}

bool nightEndScreen(GameMode mode) {
    return nightEndScreenOf(mode);
}

void armNightEndSting(NightEndSting& sting) {
    sting.secondsLeft = NIGHT_END_DELAY_SECONDS;
}

bool advanceNightEndSting(NightEndSting& sting, GameMode mode, float frameSeconds) {
    if (sting.secondsLeft < 0.0F) {
        return false;
    }
    if (!nightEndScreen(mode)) {
        sting.secondsLeft = -1.0F;
        return false;
    }
    sting.secondsLeft -= frameSeconds;
    if (sting.secondsLeft > 0.0F) {
        return false;
    }
    sting.secondsLeft = -1.0F;
    return true;
}

} // namespace game
