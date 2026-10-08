# The shade wanders, hunts by sound and sight, and the lamp burns it away
Status: accepted (2026-10-08)
Code: src/game/Shade.hpp, src/game/Shade.cpp, assets/audio/shade_alert.wav, assets/audio/shade_banish.wav

Context: the round could not be lost, and I wanted an enemy that makes the flashlight the tool of defence without a loss screen.
Decision: one shade per maze wanders slowly to far cells its own seeded generator picks. It walks to the cell a noise came from, measured along the passages: a sprint carries 14 m, a walk 6. It chases at 4 m/s only a player it sees down a corridor, and wanders again after 6 s with nothing. Lit, it stands, as before. 2.5 s of beam banish it to a far cell for 20 quiet seconds, at three times the battery drain.
Why: I manage it by being quiet and by spending battery, 4 to 6 % per banish.
Cost and revisit: ignoring the alert sound still gets a player caught. I retune by playing. Earlier: it always knew where the player was and followed whenever unlit. Two of us played that and every night became run and flash.
