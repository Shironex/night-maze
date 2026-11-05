// Tests of game/MenuBackground.hpp: the rule that chooses what is behind the main menu.
#include "game/MenuBackground.hpp"

#include "game/GameState.hpp"

#include <doctest/doctest.h>

#include <initializer_list>
#include <string>

using game::MenuBackground;

TEST_CASE("the video is shown when it was asked for and plays") {
    for (const bool stillLoaded : {false, true}) {
        const game::MenuBackgroundChoice choice =
            game::chooseMenuBackground(MenuBackground::Video, true, stillLoaded, "");
        CHECK(choice.background == MenuBackground::Video);
        CHECK(choice.reason == "the video plays");
    }
}

TEST_CASE("a video that cannot be played falls back to the still and says why") {
    const game::MenuBackgroundChoice choice = game::chooseMenuBackground(
        MenuBackground::Video, false, true, "the file is missing: menu_loop.mp4");
    CHECK(choice.background == MenuBackground::Still);
    // The error of the player is part of the reason, so the log names the cause.
    CHECK(choice.reason.find("the file is missing: menu_loop.mp4") != std::string::npos);

    // Without an error text the reason is still a sentence.
    const game::MenuBackgroundChoice silent =
        game::chooseMenuBackground(MenuBackground::Video, false, true, "");
    CHECK(silent.background == MenuBackground::Still);
    CHECK(silent.reason == "the video cannot be played");
}

TEST_CASE("without the video and without the still the live scene is shown") {
    const game::MenuBackgroundChoice choice =
        game::chooseMenuBackground(MenuBackground::Video, false, false, "no decoder");
    CHECK(choice.background == MenuBackground::LiveScene);
    // Both failures are named.
    CHECK(choice.reason.find("no decoder") != std::string::npos);
    CHECK(choice.reason.find("still image") != std::string::npos);

    // The same when the still was asked for and its file is gone.
    CHECK(game::chooseMenuBackground(MenuBackground::Still, false, false, "").background ==
          MenuBackground::LiveScene);
}

TEST_CASE("the still and the live scene can be asked for, whatever the video does") {
    for (const bool videoPlays : {false, true}) {
        const game::MenuBackgroundChoice still =
            game::chooseMenuBackground(MenuBackground::Still, videoPlays, true, "");
        CHECK(still.background == MenuBackground::Still);
        CHECK(still.reason == "asked for on the command line");

        for (const bool stillLoaded : {false, true}) {
            const game::MenuBackgroundChoice scene =
                game::chooseMenuBackground(MenuBackground::LiveScene, videoPlays, stillLoaded, "");
            CHECK(scene.background == MenuBackground::LiveScene);
            CHECK(scene.reason == "asked for on the command line");
        }
    }
}

TEST_CASE("the video and the still cover the window, the live scene is the scene") {
    CHECK(game::coversWindow(MenuBackground::Video));
    CHECK(game::coversWindow(MenuBackground::Still));
    CHECK_FALSE(game::coversWindow(MenuBackground::LiveScene));

    // What that means for a frame: behind the main menu the scene is drawn only for
    // the live scene, and a round is always drawn.
    using game::GameMode;
    CHECK_FALSE(game::drawsScene(GameMode::MainMenu, game::coversWindow(MenuBackground::Video)));
    CHECK_FALSE(game::drawsScene(GameMode::MainMenu, game::coversWindow(MenuBackground::Still)));
    CHECK(game::drawsScene(GameMode::MainMenu, game::coversWindow(MenuBackground::LiveScene)));
    CHECK(game::drawsScene(GameMode::Playing, game::coversWindow(MenuBackground::Video)));
}

TEST_CASE("every background has a name, and the name is read back") {
    for (const MenuBackground background :
         {MenuBackground::Video, MenuBackground::Still, MenuBackground::LiveScene}) {
        MenuBackground read = MenuBackground::Video;
        CHECK(game::parseMenuBackground(game::menuBackgroundName(background), read));
        CHECK(read == background);
    }
    CHECK(std::string(game::menuBackgroundName(MenuBackground::LiveScene)) == "scene");

    // Anything else is not a name, and leaves the value alone.
    MenuBackground untouched = MenuBackground::Still;
    CHECK_FALSE(game::parseMenuBackground("live", untouched));
    CHECK_FALSE(game::parseMenuBackground("", untouched));
    CHECK_FALSE(game::parseMenuBackground("Video", untouched));
    CHECK(untouched == MenuBackground::Still);
}
