// Tests of game/RoundMusic.hpp: when the music bed of a round, the layer of the chase and
// the sting of a won night are heard, and how the bed is ducked under the chase.
#include "game/RoundMusic.hpp"

#include <doctest/doctest.h>

#include <string>

namespace {

using game::GameMode;

// A frame of the game at 60 frames per second.
constexpr float FRAME = 1.0F / 60.0F;

// A round that is played and has run long enough for the music bed.
game::Round runningRound() {
    game::Round round;
    round.elapsedSeconds = game::MAZE_AMBIENT_DELAY_SECONDS;
    return round;
}

// A shade that is there and past its grace time, after what is given.
game::Shade shadeAfter(game::ShadeHunt hunt) {
    game::Shade shade;
    shade.present = true;
    shade.hunt = hunt;
    return shade;
}

} // namespace

TEST_CASE("the music of a round has files of its own after the theme of the menu") {
    CHECK(game::MAZE_AMBIENT_SOUND == game::MENU_THEME_SOUND + 1);
    CHECK(game::SHADE_CHASE_SOUND == game::MENU_THEME_SOUND + 2);
    CHECK(game::NIGHT_END_SOUND == game::MENU_THEME_SOUND + 3);
    CHECK(game::FIXED_SOUND_COUNT == game::NIGHT_END_SOUND + 1);
    CHECK(std::string(game::MAZE_AMBIENT_FILE) == "audio/maze_ambient.flac");
    CHECK(std::string(game::SHADE_CHASE_FILE) == "audio/shade_chase.flac");
    CHECK(std::string(game::NIGHT_END_FILE) == "audio/night_end.flac");
}

TEST_CASE("the music bed comes a few seconds into a played round and goes for every stop") {
    const game::Round round = runningRound();
    CHECK(game::mazeAmbientPlays({.mode = GameMode::Playing}, round));

    // Not at the very start of a round: the night opens with the wind alone.
    game::Round fresh = round;
    fresh.elapsedSeconds = game::MAZE_AMBIENT_DELAY_SECONDS - 0.1F;
    CHECK_FALSE(game::mazeAmbientPlays({.mode = GameMode::Playing}, fresh));

    // Not in a catch, under the menu camera, with the window in the background, or after
    // the round is won.
    CHECK_FALSE(game::mazeAmbientPlays({.mode = GameMode::Playing, .catching = true}, round));
    CHECK_FALSE(game::mazeAmbientPlays({.mode = GameMode::Playing, .menuCamera = true}, round));
    CHECK_FALSE(game::mazeAmbientPlays({.mode = GameMode::Playing, .windowFocused = false}, round));
    game::Round won = round;
    won.state = game::RoundState::Won;
    CHECK_FALSE(game::mazeAmbientPlays({.mode = GameMode::Playing}, won));

    // And on no other screen: the pause and its pages, the result, the menus, the films.
    for (const GameMode mode :
         {GameMode::MainMenu, GameMode::Paused, GameMode::RoundEnd, GameMode::Quitting,
          GameMode::SettingsFromMenu, GameMode::SettingsFromPause, GameMode::Intro,
          GameMode::FreePlay, GameMode::Nights, GameMode::NewCampaign, GameMode::NightCard,
          GameMode::EndingCard, GameMode::CampaignIntro, GameMode::LedgerFromMenu,
          GameMode::LedgerFromPause, GameMode::VillageBeat}) {
        CAPTURE(static_cast<int>(mode));
        CHECK_FALSE(game::mazeAmbientPlays({.mode = mode}, round));
        CHECK_FALSE(game::shadeChasePlays({.mode = mode}, round));
    }
}

TEST_CASE("the chase layer follows the hunt of the shade, not its light") {
    using game::ShadeHunt;
    CHECK_FALSE(game::shadeHunts(game::Shade{}));
    CHECK_FALSE(game::shadeHunts(shadeAfter(ShadeHunt::Wandering)));
    CHECK(game::shadeHunts(shadeAfter(ShadeHunt::Investigating)));
    CHECK(game::shadeHunts(shadeAfter(ShadeHunt::Chasing)));

    // The lamp holds a chasing shade for a moment: still a hunt, so the layer does not
    // flap 2 s in and 4 s out every time the beam crosses it.
    game::Shade lit = shadeAfter(ShadeHunt::Chasing);
    lit.lit = true;
    lit.thawLeft = 1.0F;
    CHECK(game::shadeHunts(lit));

    // Burned away and quiet, or standing in its grace time: no hunt.
    game::Shade quiet = shadeAfter(ShadeHunt::Chasing);
    quiet.quietLeft = 5.0F;
    CHECK_FALSE(game::shadeHunts(quiet));
    game::Shade grace = shadeAfter(ShadeHunt::Investigating);
    grace.graceLeft = 1.0F;
    CHECK_FALSE(game::shadeHunts(grace));

    // In a round: right from the start of the round (no delay like the bed), but not in
    // a catch.
    game::Round round;
    round.shade = shadeAfter(ShadeHunt::Chasing);
    CHECK(game::shadeChasePlays({.mode = GameMode::Playing}, round));
    CHECK_FALSE(game::shadeChasePlays({.mode = GameMode::Playing, .catching = true}, round));
    round.shade.hunt = ShadeHunt::Wandering;
    CHECK_FALSE(game::shadeChasePlays({.mode = GameMode::Playing}, round));
}

TEST_CASE("the bed ducks under the chase and comes back, step by step") {
    // Down the whole way in MAZE_AMBIENT_DUCK_SECONDS, never below the ducked volume.
    float volume = 1.0F;
    int frames = 0;
    while (volume > game::MAZE_AMBIENT_DUCKED_VOLUME) {
        const float next = game::advanceAmbientDuck(volume, true, FRAME);
        // No frame moves it by more than a hundredth: no jump that could click.
        CHECK(volume - next <= 0.011F);
        volume = next;
        ++frames;
        REQUIRE(frames < 1000);
    }
    CHECK(volume == game::MAZE_AMBIENT_DUCKED_VOLUME);
    CHECK(static_cast<float>(frames) * FRAME ==
          doctest::Approx(game::MAZE_AMBIENT_DUCK_SECONDS).epsilon(0.02));
    CHECK(game::advanceAmbientDuck(volume, true, FRAME) == game::MAZE_AMBIENT_DUCKED_VOLUME);

    // Up again, never above 1, and a long frame does not overshoot either.
    CHECK(game::advanceAmbientDuck(volume, false, 10.0F) == 1.0F);
    CHECK(game::advanceAmbientDuck(1.0F, false, FRAME) == 1.0F);
    CHECK(game::advanceAmbientDuck(0.5F, false, -1.0F) == 0.5F);
    // The bed is ducked under the chase, not silenced.
    CHECK(game::MAZE_AMBIENT_DUCKED_VOLUME > 0.2F);
    CHECK(game::MAZE_AMBIENT_DUCKED_VOLUME < 0.6F);
}

TEST_CASE("the sting of a won night comes once, after a short wait, on the screens after the win") {
    CHECK(game::nightEndScreen(GameMode::RoundEnd));
    CHECK(game::nightEndScreen(GameMode::VillageBeat));
    CHECK(game::nightEndScreen(GameMode::EndingCard));
    for (const GameMode mode :
         {GameMode::MainMenu, GameMode::Playing, GameMode::Paused, GameMode::NightCard,
          GameMode::FreePlay, GameMode::Nights, GameMode::Intro, GameMode::CampaignIntro}) {
        CHECK_FALSE(game::nightEndScreen(mode));
    }

    // Nothing armed: nothing comes.
    game::NightEndSting sting;
    CHECK_FALSE(game::advanceNightEndSting(sting, GameMode::RoundEnd, 10.0F));

    // Armed by the win: silent for NIGHT_END_DELAY_SECONDS, then once.
    game::armNightEndSting(sting);
    int due = 0;
    float seconds = 0.0F;
    float dueAt = -1.0F;
    while (seconds < 5.0F) {
        seconds += FRAME;
        if (game::advanceNightEndSting(sting, GameMode::RoundEnd, FRAME)) {
            ++due;
            dueAt = seconds;
        }
    }
    CHECK(due == 1);
    CHECK(dueAt == doctest::Approx(game::NIGHT_END_DELAY_SECONDS).epsilon(0.02));

    // A result screen that is left before the wait is over drops the sting: the next
    // night does not start with it.
    game::armNightEndSting(sting);
    CHECK_FALSE(game::advanceNightEndSting(sting, GameMode::RoundEnd, 0.5F));
    CHECK_FALSE(game::advanceNightEndSting(sting, GameMode::NightCard, FRAME));
    CHECK_FALSE(game::advanceNightEndSting(sting, GameMode::RoundEnd, 10.0F));

    // After the last night the screen moves on to the village while it waits.
    game::armNightEndSting(sting);
    CHECK_FALSE(game::advanceNightEndSting(sting, GameMode::VillageBeat, 1.0F));
    CHECK(game::advanceNightEndSting(sting, GameMode::VillageBeat, 1.0F));
}
