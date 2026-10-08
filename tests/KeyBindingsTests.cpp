// Tests of game/KeyBindings: the actions of the player, their keys, the names of the
// keys, and the lines of the settings file that hold them.
#include "game/KeyBindings.hpp"

#include "game/Settings.hpp"

#include <GLFW/glfw3.h>
#include <doctest/doctest.h>

#include <set>
#include <string>

namespace {

using game::KeyAction;
using game::KeyBindings;

// True when no two actions are on the same key.
bool allKeysDiffer(const KeyBindings& bindings) {
    const std::set<int> keys(bindings.begin(), bindings.end());
    return keys.size() == bindings.size();
}

} // namespace

TEST_CASE("the default bindings are the keys the game always had") {
    const KeyBindings keys = game::defaultKeyBindings();
    CHECK(game::boundKey(keys, KeyAction::Forward) == GLFW_KEY_W);
    CHECK(game::boundKey(keys, KeyAction::Back) == GLFW_KEY_S);
    CHECK(game::boundKey(keys, KeyAction::Left) == GLFW_KEY_A);
    CHECK(game::boundKey(keys, KeyAction::Right) == GLFW_KEY_D);
    CHECK(game::boundKey(keys, KeyAction::Sprint) == GLFW_KEY_LEFT_SHIFT);
    CHECK(game::boundKey(keys, KeyAction::Use) == GLFW_KEY_E);
    CHECK(game::boundKey(keys, KeyAction::Flashlight) == GLFW_KEY_F);
    CHECK(game::boundKey(keys, KeyAction::Map) == GLFW_KEY_M);
    CHECK(game::boundKey(keys, KeyAction::Restart) == GLFW_KEY_R);
    CHECK(game::GameSettings{}.keys == keys);
}

TEST_CASE("the table of actions is in the order of the enum and has no gaps") {
    const auto& actions = game::keyActions();
    std::set<std::string_view> names;
    for (std::size_t index = 0; index < actions.size(); ++index) {
        CHECK(static_cast<std::size_t>(actions[index].action) == index);
        CHECK(actions[index].settingName.starts_with(game::KEY_SETTING_PREFIX));
        CHECK_FALSE(actions[index].label.empty());
        // A default key can be bound: none of them is a fixed key or a key without a name.
        CHECK(game::canBindKey(actions[index].defaultKey));
        names.insert(actions[index].settingName);
    }
    CHECK(names.size() == game::KEY_ACTION_COUNT);
    CHECK(allKeysDiffer(game::defaultKeyBindings()));
    // std::string: doctest cannot print a string_view when a check fails.
    CHECK(std::string(game::keyActionInfo(KeyAction::Sprint).settingName) == "key_sprint");
    CHECK(std::string(game::keyActionInfo(KeyAction::Sprint).label) == "Sprint");
}

TEST_CASE("a key has a short name of our own") {
    CHECK(game::keyName(GLFW_KEY_W) == "W");
    CHECK(game::keyName(GLFW_KEY_4) == "4");
    CHECK(game::keyName(GLFW_KEY_LEFT_SHIFT) == "Left Shift");
    CHECK(game::keyName(GLFW_KEY_SPACE) == "Space");
    CHECK(game::keyName(GLFW_KEY_UP) == "Up");
    CHECK(game::keyName(GLFW_KEY_F) == "F");
    CHECK(game::keyName(GLFW_KEY_F1) == "F1");
    CHECK(game::keyName(GLFW_KEY_F12) == "F12");
    CHECK(game::keyName(GLFW_KEY_KP_4) == "Num 4");
    CHECK(game::keyName(GLFW_KEY_KP_ENTER) == "Num Enter");
    CHECK(game::keyName(GLFW_KEY_RIGHT_CONTROL) == "Right Ctrl");
    // Keys without a name: they cannot be bound.
    CHECK(game::keyName(GLFW_KEY_CAPS_LOCK).empty());
    CHECK(game::keyName(GLFW_KEY_LEFT_SUPER).empty());
    CHECK(game::keyName(GLFW_KEY_F13).empty());
    CHECK(game::keyName(game::NO_KEY).empty());
    CHECK(game::keyName(0).empty());
    CHECK(game::keyName(GLFW_KEY_LAST + 1).empty());
}

TEST_CASE("every name belongs to one key and leads back to it") {
    std::set<std::string> names;
    int named = 0;
    for (int key = 0; key <= GLFW_KEY_LAST; ++key) {
        const std::string name = game::keyName(key);
        if (name.empty()) {
            continue;
        }
        ++named;
        names.insert(name);
        CHECK(game::keyFromName(name) == key);
        // The menus write a name as RML and the file trims the spaces at the ends of
        // a value: a name has neither of the two signs of RML nor a space at an end.
        CHECK(name.find_first_of("<&") == std::string::npos);
        CHECK(name.front() != ' ');
        CHECK(name.back() != ' ');
    }
    // No two keys share a name ("4" and "Num 4" are two names).
    CHECK(static_cast<int>(names.size()) == named);
    // 26 letters, 10 digits, 12 function keys, 10 digits of the number pad and the 38
    // keys of the table.
    CHECK(named == 96);
}

TEST_CASE("a name is read whatever its capital letters are, and nothing else is") {
    CHECK(game::keyFromName("Left Shift") == GLFW_KEY_LEFT_SHIFT);
    CHECK(game::keyFromName("left shift") == GLFW_KEY_LEFT_SHIFT);
    CHECK(game::keyFromName("w") == GLFW_KEY_W);
    CHECK(game::keyFromName("num 4") == GLFW_KEY_KP_4);
    CHECK(game::keyFromName("") == game::NO_KEY);
    CHECK(game::keyFromName("Shift") == game::NO_KEY);
    CHECK(game::keyFromName("WW") == game::NO_KEY);
    CHECK(game::keyFromName("Caps Lock") == game::NO_KEY);
}

TEST_CASE("the fixed keys cannot be bound") {
    for (const int key :
         {GLFW_KEY_ESCAPE, GLFW_KEY_UP, GLFW_KEY_DOWN, GLFW_KEY_LEFT, GLFW_KEY_RIGHT, GLFW_KEY_TAB,
          GLFW_KEY_ENTER, GLFW_KEY_KP_ENTER, GLFW_KEY_F2, GLFW_KEY_N, GLFW_KEY_GRAVE_ACCENT}) {
        CHECK(game::isFixedKey(key));
        CHECK_FALSE(game::canBindKey(key));
        // Each has a name, so the screen can say which key was refused.
        CHECK_FALSE(game::keyName(key).empty());
    }
    CHECK_FALSE(game::isFixedKey(GLFW_KEY_W));
    CHECK(game::canBindKey(GLFW_KEY_W));
    // Space and Left Shift fly up and down in noclip, a tool. They stay free keys.
    CHECK(game::canBindKey(GLFW_KEY_SPACE));
    CHECK(game::canBindKey(GLFW_KEY_LEFT_SHIFT));
    // No name, no binding.
    CHECK_FALSE(game::canBindKey(GLFW_KEY_CAPS_LOCK));
    CHECK_FALSE(game::canBindKey(game::NO_KEY));

    KeyBindings keys = game::defaultKeyBindings();
    CHECK_FALSE(game::bindKey(keys, KeyAction::Use, GLFW_KEY_ESCAPE).accepted);
    CHECK_FALSE(game::bindKey(keys, KeyAction::Use, GLFW_KEY_N).accepted);
    CHECK_FALSE(game::bindKey(keys, KeyAction::Use, GLFW_KEY_CAPS_LOCK).accepted);
    CHECK(keys == game::defaultKeyBindings());
}

TEST_CASE("a conflict is found: the action that is on a key") {
    const KeyBindings keys = game::defaultKeyBindings();
    CHECK(game::actionOnKey(keys, GLFW_KEY_E) == KeyAction::Use);
    CHECK(game::actionOnKey(keys, GLFW_KEY_LEFT_SHIFT) == KeyAction::Sprint);
    CHECK_FALSE(game::actionOnKey(keys, GLFW_KEY_C).has_value());
}

TEST_CASE("a free key is taken, and the old key is free afterwards") {
    KeyBindings keys = game::defaultKeyBindings();
    const game::BindResult result = game::bindKey(keys, KeyAction::Sprint, GLFW_KEY_C);
    CHECK(result.accepted);
    CHECK_FALSE(result.swappedWith.has_value());
    CHECK(game::boundKey(keys, KeyAction::Sprint) == GLFW_KEY_C);
    CHECK_FALSE(game::actionOnKey(keys, GLFW_KEY_LEFT_SHIFT).has_value());
    CHECK(game::boundKeyName(keys, KeyAction::Sprint) == "C");
    // Nothing else moved.
    CHECK(game::boundKey(keys, KeyAction::Use) == GLFW_KEY_E);
    CHECK(allKeysDiffer(keys));
}

TEST_CASE("a key another action is on swaps the two") {
    KeyBindings keys = game::defaultKeyBindings();
    const game::BindResult result = game::bindKey(keys, KeyAction::Sprint, GLFW_KEY_E);
    CHECK(result.accepted);
    CHECK(result.swappedWith == KeyAction::Use);
    CHECK(game::boundKey(keys, KeyAction::Sprint) == GLFW_KEY_E);
    CHECK(game::boundKey(keys, KeyAction::Use) == GLFW_KEY_LEFT_SHIFT);
    CHECK(allKeysDiffer(keys));
    CHECK(game::keySwappedLine(keys, KeyAction::Use) == "Use is now on Left Shift.");

    // The key an action is on already: accepted, and nothing changes.
    const KeyBindings before = keys;
    const game::BindResult same = game::bindKey(keys, KeyAction::Sprint, GLFW_KEY_E);
    CHECK(same.accepted);
    CHECK_FALSE(same.swappedWith.has_value());
    CHECK(keys == before);
}

TEST_CASE("the screen says why a key was refused") {
    CHECK(game::keyRefusedLine(GLFW_KEY_N) == "N is reserved. Press another key.");
    CHECK(game::keyRefusedLine(GLFW_KEY_TAB) == "Tab is reserved. Press another key.");
    CHECK(game::keyRefusedLine(GLFW_KEY_CAPS_LOCK) ==
          "That key cannot be used. Press another key.");
}

TEST_CASE("an action is found by its settings name") {
    KeyAction action = KeyAction::Forward;
    CHECK(game::keyActionFromSetting("key_map", action));
    CHECK(action == KeyAction::Map);
    CHECK_FALSE(game::keyActionFromSetting("key_jump", action));
    CHECK_FALSE(game::keyActionFromSetting("map", action));
    CHECK(action == KeyAction::Map);
}

TEST_CASE("a key line in the file sets the key of its action") {
    game::GameSettings settings = game::parseSettings("key_sprint = C\nkey_map = Tab\n");
    CHECK(game::boundKey(settings.keys, KeyAction::Sprint) == GLFW_KEY_C);
    // Tab is a fixed key: that line is skipped.
    CHECK(game::boundKey(settings.keys, KeyAction::Map) == GLFW_KEY_M);

    CHECK(game::applySetting(settings, "key_use", "Right Ctrl"));
    CHECK(game::boundKey(settings.keys, KeyAction::Use) == GLFW_KEY_RIGHT_CONTROL);
    const game::GameSettings before = settings;
    CHECK_FALSE(game::applySetting(settings, "key_use", "Escape"));
    CHECK_FALSE(game::applySetting(settings, "key_use", ""));
    CHECK_FALSE(game::applySetting(settings, "key_jump", "J"));
    CHECK(settings == before);
}

TEST_CASE("a name the game does not know leaves the action on its default key") {
    const game::GameSettings settings =
        game::parseSettings("key_forward = Banana\nkey_use = Mouse 4\nkey_flashlight = L\n");
    CHECK(game::boundKey(settings.keys, KeyAction::Forward) == GLFW_KEY_W);
    CHECK(game::boundKey(settings.keys, KeyAction::Use) == GLFW_KEY_E);
    // The line that can be read still counts.
    CHECK(game::boundKey(settings.keys, KeyAction::Flashlight) == GLFW_KEY_L);
}

TEST_CASE("changed keys are written by name and read back the same") {
    game::GameSettings settings;
    // A free key, a swap of two actions and a round of three.
    game::bindKey(settings.keys, KeyAction::Flashlight, GLFW_KEY_KP_4);
    game::bindKey(settings.keys, KeyAction::Sprint, GLFW_KEY_E);
    game::bindKey(settings.keys, KeyAction::Forward, GLFW_KEY_A);
    game::bindKey(settings.keys, KeyAction::Left, GLFW_KEY_D);

    const std::string text = game::formatSettings(settings);
    CHECK(text.find("key_flashlight = Num 4\n") != std::string::npos);
    CHECK(text.find("key_sprint = E\n") != std::string::npos);
    CHECK(text.find("key_use = Left Shift\n") != std::string::npos);
    CHECK(game::parseSettings(text) == settings);
    CHECK(game::formatSettings(game::parseSettings(text)) == text);
}

TEST_CASE("a file that puts two actions on one key still gives every action its own") {
    // Written by hand: sprint asks for the key of forward. The two swap, like on the
    // screen, so forward ends on the key sprint had.
    const game::GameSettings one = game::parseSettings("key_sprint = W\n");
    CHECK(game::boundKey(one.keys, KeyAction::Sprint) == GLFW_KEY_W);
    CHECK(game::boundKey(one.keys, KeyAction::Forward) == GLFW_KEY_LEFT_SHIFT);
    CHECK(allKeysDiffer(one.keys));

    // Two lines with the same key: the last one has it.
    const game::GameSettings two = game::parseSettings("key_use = C\nkey_map = C\n");
    CHECK(game::boundKey(two.keys, KeyAction::Map) == GLFW_KEY_C);
    CHECK(game::boundKey(two.keys, KeyAction::Use) == GLFW_KEY_M);
    CHECK(allKeysDiffer(two.keys));
}
