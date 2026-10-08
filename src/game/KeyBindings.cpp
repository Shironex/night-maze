// Key bindings: the actions of the player, the key each one is on, and the names of
// the keys.
#include "game/KeyBindings.hpp"

// Only for the numbers of the keys (GLFW_KEY_W and the others): nothing here opens
// a window or calls a function of GLFW.
#include <GLFW/glfw3.h>

#include <algorithm>

namespace game {

namespace {

constexpr std::array<KeyActionInfo, KEY_ACTION_COUNT> KEY_ACTIONS = {{
    {.action = KeyAction::Forward,
     .settingName = "key_forward",
     .label = "Forward",
     .defaultKey = GLFW_KEY_W},
    {.action = KeyAction::Back,
     .settingName = "key_back",
     .label = "Back",
     .defaultKey = GLFW_KEY_S},
    {.action = KeyAction::Left,
     .settingName = "key_left",
     .label = "Left",
     .defaultKey = GLFW_KEY_A},
    {.action = KeyAction::Right,
     .settingName = "key_right",
     .label = "Right",
     .defaultKey = GLFW_KEY_D},
    {.action = KeyAction::Sprint,
     .settingName = "key_sprint",
     .label = "Sprint",
     .defaultKey = GLFW_KEY_LEFT_SHIFT},
    {.action = KeyAction::Use, .settingName = "key_use", .label = "Use", .defaultKey = GLFW_KEY_E},
    {.action = KeyAction::Flashlight,
     .settingName = "key_flashlight",
     .label = "Flashlight",
     .defaultKey = GLFW_KEY_F},
    {.action = KeyAction::Map,
     .settingName = "key_map",
     .label = "Map (hold)",
     .defaultKey = GLFW_KEY_M},
    {.action = KeyAction::Restart,
     .settingName = "key_restart",
     .label = "Restart",
     .defaultKey = GLFW_KEY_R},
}};

// The keys with a fixed meaning (isFixedKey). The game reads them in other places:
// Escape in core::Application, the debug window key in main.cpp, F2 and N in
// NightMazeApp.cpp, and the keys of the menus inside RmlUi. A key that gets a fixed
// meaning somewhere has to be added here by hand.
constexpr std::array FIXED_KEYS = {
    GLFW_KEY_ESCAPE, GLFW_KEY_UP,  GLFW_KEY_DOWN,         GLFW_KEY_LEFT,
    GLFW_KEY_RIGHT,  GLFW_KEY_TAB, GLFW_KEY_ENTER,        GLFW_KEY_KP_ENTER,
    GLFW_KEY_F2,     GLFW_KEY_N,   GLFW_KEY_GRAVE_ACCENT,
};

// A key and its name, for the keys whose name is not built from their number (keyName).
struct NamedKey {
    int key;
    std::string_view name;
};

constexpr std::array NAMED_KEYS = {
    NamedKey{.key = GLFW_KEY_SPACE, .name = "Space"},
    NamedKey{.key = GLFW_KEY_APOSTROPHE, .name = "Apostrophe"},
    NamedKey{.key = GLFW_KEY_COMMA, .name = "Comma"},
    NamedKey{.key = GLFW_KEY_MINUS, .name = "Minus"},
    NamedKey{.key = GLFW_KEY_PERIOD, .name = "Period"},
    NamedKey{.key = GLFW_KEY_SLASH, .name = "Slash"},
    NamedKey{.key = GLFW_KEY_SEMICOLON, .name = "Semicolon"},
    NamedKey{.key = GLFW_KEY_EQUAL, .name = "Equals"},
    NamedKey{.key = GLFW_KEY_LEFT_BRACKET, .name = "Left Bracket"},
    NamedKey{.key = GLFW_KEY_BACKSLASH, .name = "Backslash"},
    NamedKey{.key = GLFW_KEY_RIGHT_BRACKET, .name = "Right Bracket"},
    NamedKey{.key = GLFW_KEY_GRAVE_ACCENT, .name = "Backquote"},
    NamedKey{.key = GLFW_KEY_ESCAPE, .name = "Escape"},
    NamedKey{.key = GLFW_KEY_ENTER, .name = "Enter"},
    NamedKey{.key = GLFW_KEY_TAB, .name = "Tab"},
    NamedKey{.key = GLFW_KEY_BACKSPACE, .name = "Backspace"},
    NamedKey{.key = GLFW_KEY_INSERT, .name = "Insert"},
    NamedKey{.key = GLFW_KEY_DELETE, .name = "Delete"},
    NamedKey{.key = GLFW_KEY_RIGHT, .name = "Right"},
    NamedKey{.key = GLFW_KEY_LEFT, .name = "Left"},
    NamedKey{.key = GLFW_KEY_DOWN, .name = "Down"},
    NamedKey{.key = GLFW_KEY_UP, .name = "Up"},
    NamedKey{.key = GLFW_KEY_PAGE_UP, .name = "Page Up"},
    NamedKey{.key = GLFW_KEY_PAGE_DOWN, .name = "Page Down"},
    NamedKey{.key = GLFW_KEY_HOME, .name = "Home"},
    NamedKey{.key = GLFW_KEY_END, .name = "End"},
    NamedKey{.key = GLFW_KEY_KP_DECIMAL, .name = "Num Decimal"},
    NamedKey{.key = GLFW_KEY_KP_DIVIDE, .name = "Num Divide"},
    NamedKey{.key = GLFW_KEY_KP_MULTIPLY, .name = "Num Multiply"},
    NamedKey{.key = GLFW_KEY_KP_SUBTRACT, .name = "Num Minus"},
    NamedKey{.key = GLFW_KEY_KP_ADD, .name = "Num Plus"},
    NamedKey{.key = GLFW_KEY_KP_ENTER, .name = "Num Enter"},
    NamedKey{.key = GLFW_KEY_LEFT_SHIFT, .name = "Left Shift"},
    NamedKey{.key = GLFW_KEY_LEFT_CONTROL, .name = "Left Ctrl"},
    NamedKey{.key = GLFW_KEY_LEFT_ALT, .name = "Left Alt"},
    NamedKey{.key = GLFW_KEY_RIGHT_SHIFT, .name = "Right Shift"},
    NamedKey{.key = GLFW_KEY_RIGHT_CONTROL, .name = "Right Ctrl"},
    NamedKey{.key = GLFW_KEY_RIGHT_ALT, .name = "Right Alt"},
};

// The place of an action in the two arrays.
std::size_t indexOf(KeyAction action) {
    return static_cast<std::size_t>(action);
}

// A letter as its small letter, everything else as it is. Only A to Z: the names of
// the keys have no other letters.
char lowered(char character) {
    return character >= 'A' && character <= 'Z' ? static_cast<char>(character - 'A' + 'a')
                                                : character;
}

// True when two texts are the same but for capital and small letters.
bool sameName(std::string_view first, std::string_view second) {
    return std::ranges::equal(first, second,
                              [](char one, char other) { return lowered(one) == lowered(other); });
}

} // namespace

const std::array<KeyActionInfo, KEY_ACTION_COUNT>& keyActions() {
    return KEY_ACTIONS;
}

const KeyActionInfo& keyActionInfo(KeyAction action) {
    return KEY_ACTIONS[indexOf(action)];
}

KeyBindings defaultKeyBindings() {
    KeyBindings bindings{};
    for (const KeyActionInfo& info : KEY_ACTIONS) {
        bindings[indexOf(info.action)] = info.defaultKey;
    }
    return bindings;
}

int boundKey(const KeyBindings& bindings, KeyAction action) {
    return bindings[indexOf(action)];
}

std::string keyName(int key) {
    // GLFW gives the letter and the digit keys the numbers of their characters: the
    // key A is 'A' (65), the key 1 is '1' (49). So the name is that one character.
    if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z) || (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)) {
        // Round brackets: one copy of the character. Curly ones would read the two
        // values as a list of two characters.
        std::string name(1, static_cast<char>(key));
        return name;
    }
    // The function keys and the digits of the number pad are numbered in a row.
    if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F12) {
        return "F" + std::to_string(key - GLFW_KEY_F1 + 1);
    }
    if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
        return "Num " + std::to_string(key - GLFW_KEY_KP_0);
    }
    for (const NamedKey& named : NAMED_KEYS) {
        if (named.key == key) {
            return std::string(named.name);
        }
    }
    return {};
}

int keyFromName(std::string_view name) {
    if (name.empty()) {
        return NO_KEY;
    }
    // The other way round through the same function, so the two can never disagree.
    // It asks every key code there is: a few hundred, once per line of the file.
    for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key) {
        if (sameName(keyName(key), name)) {
            return key;
        }
    }
    return NO_KEY;
}

std::string boundKeyName(const KeyBindings& bindings, KeyAction action) {
    return keyName(boundKey(bindings, action));
}

bool isFixedKey(int key) {
    return std::ranges::find(FIXED_KEYS, key) != FIXED_KEYS.end();
}

bool canBindKey(int key) {
    return !keyName(key).empty() && !isFixedKey(key);
}

std::optional<KeyAction> actionOnKey(const KeyBindings& bindings, int key) {
    for (const KeyActionInfo& info : KEY_ACTIONS) {
        if (boundKey(bindings, info.action) == key) {
            return info.action;
        }
    }
    return std::nullopt;
}

BindResult bindKey(KeyBindings& bindings, KeyAction action, int key) {
    if (!canBindKey(key)) {
        return {};
    }
    BindResult result{.accepted = true, .swappedWith = std::nullopt};
    const std::optional<KeyAction> holder = actionOnKey(bindings, key);
    if (holder.has_value() && *holder != action) {
        // The other action takes the key this one is about to leave.
        bindings[indexOf(*holder)] = boundKey(bindings, action);
        result.swappedWith = holder;
    }
    bindings[indexOf(action)] = key;
    return result;
}

bool keyActionFromSetting(std::string_view settingName, KeyAction& action) {
    for (const KeyActionInfo& info : KEY_ACTIONS) {
        if (info.settingName == settingName) {
            action = info.action;
            return true;
        }
    }
    return false;
}

bool applyKeySetting(KeyBindings& bindings, std::string_view settingName, std::string_view value) {
    KeyAction action = KeyAction::Forward;
    if (!keyActionFromSetting(settingName, action)) {
        return false;
    }
    return bindKey(bindings, action, keyFromName(value)).accepted;
}

std::string keyRefusedLine(int key) {
    const std::string name = keyName(key);
    if (name.empty()) {
        return "That key cannot be used. Press another key.";
    }
    return name + " is reserved. Press another key.";
}

std::string keySwappedLine(const KeyBindings& bindings, KeyAction moved) {
    return std::string(keyActionInfo(moved).label) + " is now on " + boundKeyName(bindings, moved) +
           ".";
}

} // namespace game
