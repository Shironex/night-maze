// Key bindings: the actions of the player, the key each one is on, and the names of
// the keys.
#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace game {

// Plain data and pure functions without a window, like the rest of the game_logic
// library, so tests can check every rule. A key is a whole number: the key code of GLFW
// (GLFW_KEY_W is 87). GLFW gives a key the code of the place it has on a US keyboard,
// whatever the layout of the system is, so a binding means "the key in this place".

/// What the player can put on a key of their choice. The numbers are the places in
/// a KeyBindings array and in keyActions(), so new actions go at the end.
enum class KeyAction {
    Forward = 0, ///< walk forward
    Back,        ///< walk back
    Left,        ///< step to the left
    Right,       ///< step to the right
    Sprint,      ///< run, while held
    Use,         ///< pull a lever, read a note, close its card
    Flashlight,  ///< switch the flashlight on and off
    Map,         ///< show the map, while held
    Restart,     ///< start the round again on the same maze
};
constexpr std::size_t KEY_ACTION_COUNT = 9;

/// No key at all. It is also what GLFW calls a key it does not know (GLFW_KEY_UNKNOWN).
constexpr int NO_KEY = -1;

/// One row of the table of actions.
struct KeyActionInfo {
    KeyAction action;
    /// Its name in the settings file and as the id of its button on the settings
    /// screen: "key_sprint".
    std::string_view settingName;
    /// Its name as the settings screen shows it: "Sprint".
    std::string_view label;
    /// The key it is on in a game without a settings file: the key the game always
    /// had for it.
    int defaultKey;
};

/// The table of actions, in the order of KeyAction and of the settings screen.
const std::array<KeyActionInfo, KEY_ACTION_COUNT>& keyActions();

/// The row of one action.
const KeyActionInfo& keyActionInfo(KeyAction action);

/// The key of every action: bindings[i] is the key of the action with the number i.
/// No two actions ever share a key (bindKey keeps it that way).
using KeyBindings = std::array<int, KEY_ACTION_COUNT>;

/// The bindings of a game without a settings file: W, S, A, D, Left Shift, E, F, M, R.
KeyBindings defaultKeyBindings();

/// The key an action is on.
int boundKey(const KeyBindings& bindings, KeyAction action);

/// The name of a key, short enough for a button and for a prompt of the HUD: "W",
/// "Left Shift", "Space", "Up", "F5", "Num 4". Empty for a key that has no name here
/// (Caps Lock, the Windows key and the like): such a key cannot be bound.
///
/// The table is our own. GLFW has glfwGetKeyName, but it answers nothing for keys like
/// Shift or Space, and its answer depends on the layout of the keyboard: a settings file
/// would then mean one thing on one computer and another thing on the next. The price:
/// on a keyboard that is not a US one the name can differ from the letter printed on
/// the key (the key right of Tab is "Q" here, also where it is printed A).
std::string keyName(int key);

/// The key with this name, or NO_KEY. Capital and small letters count as the same, so
/// a file written by hand may say "left shift".
int keyFromName(std::string_view name);

/// The name of the key an action is on: what the HUD prints in a prompt.
std::string boundKeyName(const KeyBindings& bindings, KeyAction action);

/// True for a key that has a fixed meaning and can never be given to an action:
/// Escape (pause and back), the keys that move through a menu (the four arrows, Tab,
/// Enter and the Enter of the number pad) and the keys of the tools (the key left of 1
/// that shows the debug window, F2 for the menu camera, N for noclip).
bool isFixedKey(int key);

/// True for a key an action can be put on: it has a name and is not fixed.
bool canBindKey(int key);

/// The action that is on this key, or nothing when the key is free. Asked before a key
/// is given away: an answer that is another action is a conflict.
std::optional<KeyAction> actionOnKey(const KeyBindings& bindings, int key);

/// What bindKey did.
struct BindResult {
    /// False: the key cannot be bound (canBindKey), and nothing was changed.
    bool accepted = false;
    /// The action that had the key before and now has the old key of the other one.
    /// Nothing when the key was free.
    std::optional<KeyAction> swappedWith;
};

/// Puts an action on a key. When another action is on that key the two swap: the other
/// one gets the key this action had. So no action is ever left without a key, and no
/// two actions share one. A key that cannot be bound changes nothing.
BindResult bindKey(KeyBindings& bindings, KeyAction action, int key);

/// The action with this settings name ("key_sprint"). False, and action unchanged,
/// when no action has that name.
bool keyActionFromSetting(std::string_view settingName, KeyAction& action);

/// Sets one binding from a line of the settings file: the settings name of an action
/// and the name of a key. It is bindKey, so a file that puts two actions on one key
/// still ends with every action on a key of its own. False, and nothing changed, when
/// the name is not an action or the value is not a key that can be bound: the action
/// then keeps the key it has, which is its default unless a swap moved it.
bool applyKeySetting(KeyBindings& bindings, std::string_view settingName, std::string_view value);

/// The line the settings screen shows when a key was refused: "N is reserved. Press
/// another key." for a fixed key, "That key cannot be used. Press another key." for
/// a key without a name.
std::string keyRefusedLine(int key);

/// The line the settings screen shows after a swap, about the action that was moved:
/// "Use is now on Left Shift."
std::string keySwappedLine(const KeyBindings& bindings, KeyAction moved);

} // namespace game
