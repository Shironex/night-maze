# Key bindings are a table of actions with key names of the game's own
Status: accepted (2026-10-08)
Code: src/game/KeyBindings.hpp, src/game/KeyBindings.cpp, src/game/Settings.hpp, assets/ui/settings.rml

Context: players use other keyboard layouts, and GLFW key codes name places on a keyboard, not letters.
Decision: nine actions are rows of a table. The settings file stores each key by a name from a table of my own. Binding a key that another action holds swaps the two actions. Escape, the menu keys and the tool keys stay fixed.
Why: storing raw GLFW codes would make the file unreadable and tie it to one library. Refusing a clash lost because it leaves the player stuck, while a swap never leaves an action without a key.
Cost and revisit: the name table must be kept in step with the keys I allow. I add names when a player asks for a key that has none.
