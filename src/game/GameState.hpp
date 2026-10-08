// GameState: which screen the game is on (menu, intro, free play, nights of the campaign,
// playing, paused, round end, settings, cards) and the rules for going from one to the next.
#pragma once

#include "game/Difficulty.hpp"
#include "game/MazeWorld.hpp"
#include "game/StartOptions.hpp"

#include <cstdint>
#include <string_view>

namespace game {

// Plain data and pure functions without a window, without OpenGL and without the menu
// library, like the rest of the game_logic library, so tests can check every rule. The
// application (NightMazeApp) holds one GameMode, feeds it events and asks the questions
// below instead of deciding by itself what a screen allows.

/// The screen the game is on. Exactly one at a time.
enum class GameMode {
    MainMenu = 0, ///< the main menu, the first thing after the start
    Playing,      ///< a round is being played
    Paused,       ///< a round is stopped, the pause menu is shown over it
    RoundEnd,     ///< the round is won, its result is shown
    Quitting,     ///< the player asked to leave: the program closes its window
    /// The settings screen, opened from the main menu. It is a screen of its own for
    /// each place it can be opened from, so the screen itself remembers where "back"
    /// leads and what is shown behind it.
    SettingsFromMenu,
    SettingsFromPause, ///< the settings screen, opened from the pause menu
    /// The intro: five cards of text over pictures of the maze, before the main menu
    /// (game/Intro.hpp). It ends by itself, and any key ends it earlier.
    Intro,
    /// The screen of free play, opened from the main menu: the difficulty, the calm
    /// night, the seed and the button that starts a game.
    FreePlay,
    /// The list of the five nights of the campaign (game/Campaign.hpp), opened from the
    /// main menu.
    Nights,
    /// The question before a new campaign replaces a finished one.
    NewCampaign,
    /// The title card of a night: its number and its name on black, before its round.
    /// It ends by itself, and any key ends it earlier.
    NightCard,
    /// The ending card: four lines on black after the last night is won, in the place
    /// of the result screen. It ends by itself, and any key ends it earlier.
    EndingCard,
};

/// Something that can change the screen: a button of a menu, the Escape key, or the
/// round itself.
enum class GameEvent {
    Play = 0,      ///< button "Play" of free play: start a new game
    Resume,        ///< button "Resume" of the pause menu
    Restart,       ///< button "Restart": the same maze again, from the start
    BackToMenu,    ///< button "Back to menu"
    Quit,          ///< button "Quit" of the main menu
    Escape,        ///< the Escape key
    RoundWon,      ///< the player walked through the open gate (RoundState::Won)
    OpenSettings,  ///< button "Settings" of the main menu and of the pause menu
    CloseSettings, ///< button "Back" of the settings screen
    NewMaze,       ///< button "New maze" of the result screen: a new game, another maze
    FocusLost,     ///< the window of the game stopped being the active window
    IntroFinished, ///< the intro reached its end, or a key or a mouse button skipped it
    OpenFreePlay,  ///< button "Free play" of the main menu
    OpenNights,    ///< button "Nights" of the main menu
    /// The first entry of the main menu while its campaign is finished ("New campaign"):
    /// the game asks before it throws the finished one away.
    AskNewCampaign,
    /// A night of the campaign is started: the first entry of the main menu ("Begin",
    /// "Continue"), a night of the list, or the answer "yes" to a new campaign. The
    /// application says which night.
    StartNight,
    CardFinished, ///< a title card or the ending card reached its end, or was skipped
    CampaignWon,  ///< the player walked through the gate of the last night
};

/// What the button "Play" asks for: the game that is started next.
struct NewGame {
    /// The level: the size of the maze, its crystals, the gate and the battery
    /// (game::difficultyLevel).
    Difficulty difficulty = Difficulty::Normal;

    /// The seed of the maze (MazeSettings::seed).
    std::uint32_t seed = DEFAULT_MAZE_SEED;
};

/// The screen after an event. An event that means nothing on a screen leaves the
/// screen as it is, so a caller can send any event at any time.
///
///     screen             event          next screen
///     MainMenu           StartNight     NightCard
///     MainMenu           AskNewCampaign NewCampaign
///     MainMenu           OpenNights     Nights
///     MainMenu           OpenFreePlay   FreePlay
///     MainMenu           OpenSettings   SettingsFromMenu
///     MainMenu           Quit           Quitting
///     FreePlay           Play           Playing
///     FreePlay           BackToMenu     MainMenu
///     FreePlay           Escape         MainMenu
///     Nights             StartNight     NightCard
///     Nights             BackToMenu     MainMenu
///     Nights             Escape         MainMenu
///     NewCampaign        StartNight     NightCard
///     NewCampaign        BackToMenu     MainMenu
///     NewCampaign        Escape         MainMenu
///     NightCard          CardFinished   Playing
///     NightCard          Escape         Playing
///     Playing            Escape         Paused
///     Playing            FocusLost      Paused
///     Playing            RoundWon       RoundEnd
///     Playing            CampaignWon    EndingCard
///     EndingCard         CardFinished   MainMenu
///     EndingCard         Escape         MainMenu
///     Paused             Escape         Playing
///     Paused             Resume         Playing
///     Paused             Restart        Playing
///     Paused             OpenSettings   SettingsFromPause
///     Paused             BackToMenu     MainMenu
///     RoundEnd           Restart        Playing
///     RoundEnd           NewMaze        Playing
///     RoundEnd           BackToMenu     MainMenu
///     RoundEnd           Escape         MainMenu
///     SettingsFromMenu   CloseSettings  MainMenu
///     SettingsFromMenu   Escape         MainMenu
///     SettingsFromPause  CloseSettings  Paused
///     SettingsFromPause  Escape         Paused
///     Intro              IntroFinished  MainMenu
///     Intro              Escape         MainMenu
///
/// Escape goes one screen back: out of the game into the pause menu, out of the pause
/// menu back into the game, out of the result to the main menu, out of the settings to
/// the screen they were opened from, out of free play, the list of nights and the
/// question about a new campaign to the main menu. In the main menu there is nothing to
/// go back to, and leaving the program is a button, not a key.
///
/// A night of the campaign begins with its title card, and the card leads into the
/// round in one direction only: Escape skips it like any other key. The last night ends
/// with the ending card in the place of the result screen, and that card leads to the
/// main menu. Like the intro, a card runs on a clock of its own and is not touched by
/// a lost focus.
///
/// The intro is left in one direction only, to the main menu. Losing the focus does not
/// touch it: it runs on a clock of its own and ends by itself, so a player who switches
/// to another program comes back to the rest of it or to the main menu, never to
/// a screen that waits for something. No event leads back into the intro: the
/// application starts it (startMode at the start, a button of the debug window later).
GameMode nextMode(GameMode mode, GameEvent event);

/// The screen the game starts on. options is what the command line asked for and
/// introSeen what the settings file says (GameSettings::introSeen). The first row that
/// fits decides:
///
///     --intro without --skip-intro         Intro, whatever else was given
///     --play, --menu-camera or --night     Playing
///     --skip-intro                         MainMenu
///     a switch of a tool (toolSwitch)      MainMenu
///     the intro was seen                   MainMenu
///     otherwise                            Intro
///
/// A run driven by a tool never opens with the intro: its scripts start the game in
/// fresh folders, where nothing says that it was seen. --skip-intro means never, also
/// next to --intro.
GameMode startMode(const StartOptions& options, bool introSeen);

/// True when the event, sent on this screen, starts a round from the beginning on the
/// maze that is in play: Restart in the pause menu or on the result screen.
bool startsRound(GameMode mode, GameEvent event);

/// True when the event, sent on this screen, starts a new game of free play: a maze is
/// built and a round starts in it. Play on the screen of free play, and NewMaze on the
/// result screen. A night of the campaign is not among them: the application builds its
/// maze before it sends StartNight.
bool startsNewGame(GameMode mode, GameEvent event);

/// The event behind the name a menu button carries in its document (the attribute
/// data-action): "play", "resume", "restart", "menu", "quit", "settings", "back",
/// "new-maze", "free-play" or "nights". False for any other name: event is left as it was. The
/// Escape key, the won round and the lost focus are not buttons and have no name.
bool eventForAction(std::string_view action, GameEvent& event);

/// True while the rules of the round run: the player moves, the battery drains,
/// crystals are collected and the time counts. Only while playing.
bool updatesRound(GameMode mode);

/// True while the things that move by themselves keep moving (bobbing crystals, pulsing
/// lights). They stand still only in the pause menu and in the settings opened from it:
/// a pause stops everything.
bool animatesScene(GameMode mode);

/// True while a menu document is shown. The cursor is then free and the mouse and the
/// keys of the round do not reach the game.
bool isMenuOpen(GameMode mode);

/// True for a screen that is a film: the intro, the title card of a night and the ending
/// card. It has no cursor and no debug window in its picture, nothing can be clicked, and
/// any key ends it.
bool isFilm(GameMode mode);

/// True while the HUD of the round is drawn (counter, battery, crosshair, cards): only
/// while playing. It is drawn after the menu documents, so in the pause menu it would
/// lie on top of the buttons. The intro has no HUD, no crosshair and no map.
bool showsHud(GameMode mode);

/// What decides whether the map is on the screen, besides the screen the game is on.
struct MapRequest {
    /// The map key is down in this moment. The map is held, not switched: it is gone
    /// when the key is released.
    bool keyHeld = false;

    /// The debug switch that keeps the map open without the key (MinimapSettings::pinned).
    bool pinned = false;

    /// The card of a note is open (Round::noteOpen).
    bool noteOpen = false;

    /// The menu camera shows the game (MenuCameraSettings::enabled): its picture has no
    /// HUD and no map.
    bool menuCamera = false;
};

/// True while the map is on the screen: a round is being played, and the map key is
/// held or the debug switch pins the map.
///
///   - Only while playing: the pause menu and the result screen take the map away, and
///     so does a window that stops being the active one, because that pauses the round
///     (FocusLost).
///   - Not while the card of a note is open. The card came first and stays: the player
///     closes it, and then the key shows the map.
///   - Not in the picture of the menu camera.
///
/// While the map is shown the player stands still and does not look around
/// (game::movementInput), and nothing can be used. The round itself goes on.
bool showsMap(GameMode mode, const MapRequest& request);

/// True while the picture is taken by the menu camera and not from the eyes of the
/// player: in the main menu, which has no round to show, and on the screens opened from
/// it (the settings, free play, the list of nights, the question about a new campaign). The intro
/// is not among them: it places the camera itself, shot by shot (game/Intro.hpp), and its scene is
/// always drawn.
bool usesMenuCamera(GameMode mode);

/// True when the scene has to be drawn. fullscreenBackground tells whether the menu
/// has a picture of its own that covers the whole window (a video). The main menu with
/// such a background hides the scene completely, so its passes can be left out, and so
/// do the settings opened from it. Every other screen shows the scene, also behind
/// a menu.
bool drawsScene(GameMode mode, bool fullscreenBackground);

} // namespace game
