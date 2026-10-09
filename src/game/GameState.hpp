// GameState: which screen the game is on (menu, intro, free play, nights of the campaign,
// playing, paused, round end, settings, ledger, cards) and the rules for going from one to the
// next.
#pragma once

#include "game/Campaign.hpp"
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
    /// The intro played by itself: five cards of text over pictures of the maze
    /// (game/Intro.hpp), asked for by the switch --intro or by the debug window. It ends
    /// by itself, and any key ends it earlier. The main menu follows.
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
    /// It ends by itself, and any key ends it earlier. The maze of the day has the same
    /// card, with its name and its day.
    NightCard,
    /// The ending card: four lines on black after the last night is won, in the place
    /// of the result screen. It ends by itself, and any key ends it earlier.
    EndingCard,
    /// The intro at the beginning of a campaign: the same five cards, and the title card
    /// of the first night follows. It is a screen of its own for the same reason as the
    /// two settings screens: the screen itself remembers where its end leads.
    CampaignIntro,
    /// The lamplighter's ledger (game/Ledger.hpp): the page with every line of the story
    /// the player has read. Like the settings it is a screen of its own for each place
    /// it can be opened from: the main menu and the pause menu.
    LedgerFromMenu,
    LedgerFromPause,
    /// The village on the ridge after the last night is won, for a few seconds before the
    /// ending card: the camera rises over the gate and the last window lights
    /// (game/Village.hpp). It ends by itself, and any key ends it earlier.
    VillageBeat,
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
    CloseSettings, ///< button "Back" of the settings screen and of the ledger
    NewMaze,       ///< button "New maze" of the result screen: a new game, another maze
    FocusLost,     ///< the window of the game stopped being the active window
    IntroFinished, ///< the intro reached its end, or a key or a mouse button skipped it
    OpenFreePlay,  ///< button "Free play" of the main menu
    OpenNights,    ///< button "Nights" of the main menu
    /// The first entry of the main menu while its campaign is finished ("New campaign"):
    /// the game asks before it throws the finished one away.
    AskNewCampaign,
    /// A night of the campaign is started: the first entry of the main menu
    /// ("Continue"), a night of the list, or the button "Next night" of the result
    /// screen. The application says which night. The entry "Tonight's hedge" of the main
    /// menu sends it too: the maze of the day begins with a title card like a night
    /// (game/Daily.hpp).
    StartNight,
    CardFinished, ///< a title card or the ending card reached its end, or was skipped
    CampaignWon,  ///< the player walked through the gate of the last night
    /// A campaign is begun from its first night with the intro before it: the entry
    /// "Begin" of the main menu, or the answer "yes" to a new campaign
    /// (campaignEntryEvent).
    BeginCampaign,
    OpenLedger, ///< button "Ledger" of the main menu and of the pause menu
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
///     MainMenu           BeginCampaign  CampaignIntro
///     MainMenu           StartNight     NightCard
///     MainMenu           AskNewCampaign NewCampaign
///     MainMenu           OpenNights     Nights
///     MainMenu           OpenFreePlay   FreePlay
///     MainMenu           OpenSettings   SettingsFromMenu
///     MainMenu           OpenLedger     LedgerFromMenu
///     MainMenu           Quit           Quitting
///     FreePlay           Play           Playing
///     FreePlay           BackToMenu     MainMenu
///     FreePlay           Escape         MainMenu
///     Nights             StartNight     NightCard
///     Nights             BackToMenu     MainMenu
///     Nights             Escape         MainMenu
///     NewCampaign        BeginCampaign  CampaignIntro
///     NewCampaign        StartNight     NightCard
///     NewCampaign        BackToMenu     MainMenu
///     NewCampaign        Escape         MainMenu
///     NightCard          CardFinished   Playing
///     NightCard          Escape         Playing
///     Playing            Escape         Paused
///     Playing            FocusLost      Paused
///     Playing            RoundWon       RoundEnd
///     Playing            CampaignWon    VillageBeat
///     VillageBeat        CardFinished   EndingCard
///     VillageBeat        Escape         EndingCard
///     EndingCard         CardFinished   MainMenu
///     EndingCard         Escape         MainMenu
///     Paused             Escape         Playing
///     Paused             Resume         Playing
///     Paused             Restart        Playing
///     Paused             OpenSettings   SettingsFromPause
///     Paused             OpenLedger     LedgerFromPause
///     Paused             BackToMenu     MainMenu
///     RoundEnd           Restart        Playing
///     RoundEnd           NewMaze        Playing
///     RoundEnd           StartNight     NightCard
///     RoundEnd           OpenNights     Nights
///     RoundEnd           BackToMenu     MainMenu
///     RoundEnd           Escape         MainMenu
///     SettingsFromMenu   CloseSettings  MainMenu
///     SettingsFromMenu   Escape         MainMenu
///     SettingsFromPause  CloseSettings  Paused
///     SettingsFromPause  Escape         Paused
///     LedgerFromMenu     CloseSettings  MainMenu
///     LedgerFromMenu     Escape         MainMenu
///     LedgerFromPause    CloseSettings  Paused
///     LedgerFromPause    Escape         Paused
///     Intro              IntroFinished  MainMenu
///     Intro              Escape         MainMenu
///     CampaignIntro      IntroFinished  NightCard
///     CampaignIntro      Escape         NightCard
///
/// Escape goes one screen back: out of the game into the pause menu, out of the pause
/// menu back into the game, out of the result to the main menu, out of the settings to
/// the screen they were opened from, out of the ledger likewise, out of free play, the list of
/// nights and the question about a new campaign to the main menu. In the main menu there is nothing
/// to go back to, and leaving the program is a button, not a key.
///
/// A night of the campaign begins with its title card, and the card leads into the
/// round in one direction only: Escape skips it like any other key. The result screen of
/// a night leads on to the next night ("Next night", StartNight) or to the list of
/// nights ("Back to nights", OpenNights): which of the two it offers is a rule of the
/// campaign (game::nightEndOffer). The last night ends
/// with a look at the village and then the ending card in the place of the result screen,
/// and that card leads to the main menu. Like the intro, a card runs on a clock of its own and is
/// not touched by a lost focus.
///
/// The intro belongs to the campaign: it plays when a campaign begins (BeginCampaign),
/// and the title card of the first night follows it. It is left in one direction only,
/// at its end or by a key, and Escape is such a key. Losing the focus does not touch it:
/// it runs on a clock of its own and ends by itself, so a player who switches to another
/// program comes back to the rest of it or to what follows, never to a screen that waits
/// for something. The intro played by itself (Intro) ends in the main menu, and no event
/// leads into it: the application starts it (startMode at the start, a button of the
/// debug window later).
GameMode nextMode(GameMode mode, GameEvent event);

/// The screen the game starts on. options is what the command line asked for. The first
/// row that fits decides:
///
///     --intro without --skip-intro         Intro, whatever else was given
///     --play, --menu-camera, --night or --daily   Playing
///     otherwise                            MainMenu
///
/// The game opens with the main menu, also on its very first start: the intro waits for
/// the campaign (campaignEntryEvent). The settings file is not asked. --intro plays the
/// intro at the start all the same, for tools, and --skip-intro means never, also next
/// to --intro.
GameMode startMode(const StartOptions& options);

/// True when a campaign that is begun in this run plays the intro before its first
/// night. Not with --skip-intro, and not in a run driven by a tool
/// (StartOptions::toolSwitch): a script that starts a night wants the night.
bool campaignIntroPlays(const StartOptions& options);

/// What the first entry of the main menu does, for a campaign that is this far.
/// introPlays is campaignIntroPlays of the run.
///
///     NotStarted ("Begin")         BeginCampaign: the intro, then the first night.
///                                  StartNight when the intro does not play
///     Running ("Continue")         StartNight: the next night, never the intro
///     Finished ("New campaign")    AskNewCampaign: the question comes first
///
/// The answer "yes" to that question forgets the finished campaign, and what follows is
/// the row NotStarted. A night of the list of nights is always StartNight.
GameEvent campaignEntryEvent(CampaignStage stage, bool introPlays);

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
/// "new-maze", "free-play", "nights" or "ledger". False for any other name: event is left as it
/// was. The Escape key, the won round and the lost focus are not buttons and have no name.
bool eventForAction(std::string_view action, GameEvent& event);

/// True while the rules of the round run: the player moves, the battery drains,
/// crystals are collected and the time counts. Only while playing.
bool updatesRound(GameMode mode);

/// True while the things that move by themselves keep moving (bobbing crystals, pulsing
/// lights). They stand still only in the pause menu and in the settings and the ledger
/// opened from it: a pause stops everything.
bool animatesScene(GameMode mode);

/// True while a menu document is shown. The cursor is then free and the mouse and the
/// keys of the round do not reach the game.
bool isMenuOpen(GameMode mode);

/// True for the two screens that play the intro: by itself (Intro) and at the beginning
/// of a campaign (CampaignIntro).
bool isIntro(GameMode mode);

/// True for a screen that is a film: the intro, the title card of a night, the look at
/// the village after the last night and the ending card. It has no cursor and no debug window in
/// its picture, nothing can be clicked, and any key ends it.
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
/// it (the settings, the ledger, free play, the list of nights, the question about a new campaign).
/// The intro is not among them: it places the camera itself, shot by shot (game/Intro.hpp), and its
/// scene is always drawn.
bool usesMenuCamera(GameMode mode);

/// True when the scene has to be drawn. fullscreenBackground tells whether the menu
/// has a picture of its own that covers the whole window (a video). The main menu with
/// such a background hides the scene completely, so its passes can be left out, and so
/// do the settings opened from it. Every other screen shows the scene, also behind
/// a menu.
bool drawsScene(GameMode mode, bool fullscreenBackground);

} // namespace game
