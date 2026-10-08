# Changelog

Release notes for Night Maze, newest first.

## 0.13.0 (2026-10-08)

The story has a path now: five nights, a shadow that has to find you, and a screen with real instruments.

### What's new

- The campaign. The main menu starts with Begin, which becomes Continue once you are on your way. It takes you through five nights, each in a bigger maze with a shorter battery: First Frost (10 x 10), The Shepherds' Gates (13 x 13), Lamp's Back (16 x 16), What the Moon Misses (19 x 19) and The Last Lamp (22 x 22). The gate asks for 70 % of the crystals in the first three nights and 80 % in the last two.
- The first night has no shadow, so you can learn the walk. The shadow comes from night 2.
- "Nights" in the main menu lists the five nights with your best time for each. You can play a finished night again. A new night opens when you win the one before it.
- Every night starts with a title card with its number and name. When you win, the result screen has "Next night" and one line of the story. After the fifth night an ending card closes the story. "New campaign" starts again from night 1 and asks first, because it clears your finished nights and best times.
- Free play keeps the old menu: difficulty, calm night and seed. Calm night only changes free play.
- The intro of five cards now plays when a campaign begins, no longer at the first start. The game opens with the main menu. Continue and the list of nights never play it.
- The shadow works in a new way, and this replaces 0.12.0. It no longer always knows where you are. It wanders slowly through the maze. It hears you: a sprint from 14 metres, a lever from 10, a pickup from 8 and a walk from 6, counted along the corridors. It also sees you along a straight row or column of the maze, up to 12 metres. It walks to the place of a noise, runs at you when it sees you, and after 6 seconds without finding you it gives up and wanders again.
- Hold the light on the shadow for 2.5 seconds and it burns away. That costs three times the usual battery, and the shadow is moved to a far cell, where it is quiet and deaf for 20 seconds. When it reaches you, the screen now fades to black before you are back at the start. The drawn shadow also sways as it walks.
- A new HUD, in the corners. Bottom left: a ring for the lamp with the charge as a number. Top right: the crystals you hold against the number the gate needs, with a tick on the bar, and the time. At the bottom: the stamina line and, in a maze with a shadow, three ticks that show how loud you are. Short sentences appear for the tea, the open gate and an empty battery. The crosshair prompt names your key, and a night of the campaign shows its name for the first seconds. The HUD grows with the window, so it keeps its size on a large or dense screen.
- The Lamplighter's Gate. The exit is now a gatehouse with three lanterns. They are cold and blue while the gate is shut, and they light up warm when it opens. A bell tolls every 6 seconds while the gate is open, louder the closer you are, and once when you walk through. On the map, a small tick points to the open gate and a lantern marks the exit cell. No grass grows in the exit cell or the cell before it.
- More sound: wind in the maze, your footsteps, the steps of the shadow, a cold sound when it notices you, a soft breath when the light burns it away, and the bell of the gate. That makes 22 sounds. The settings screen has a volume for effects and one for ambient sound (the wind) under the main volume.
- You can choose your own keys. The Controls part of the settings screen lists forward, back, left, right, sprint, use, flashlight, map and restart. Press Enter on one, then the new key. If the key is already used, the two actions swap keys. "Reset controls" brings the defaults back, and so does "Reset defaults". The keys are saved with your settings.
- Your settings from 0.12.0 load as they were. The new volumes start at 100, the keys at their defaults, and the campaign at night 1 with no best times. Your place in the story is kept.
- The video behind the main menu is recorded again, so it shows the gatehouse.

## 0.12.0 (2026-10-08)

Something walks in the maze now, and the game opens with its story.

### What's new

- The shadow. One dark hooded figure starts far from you in every maze and comes for you. It stands still only while your flashlight is on it, with no wall between, and for two seconds after the light leaves it. Looking at it with the lamp off does not stop it. It is faster than your walk and slower than your sprint.
- You hear it before you see it: a low hum that pulses faster as it gets closer along the corridors.
- When it reaches you it carries you back to the start. The round begins again with your crystals, battery and levers reset, and a short line tells you what happened. There is still no losing screen.
- Calm night: a switch in the main menu for a maze without the shadow. It works on every difficulty and is saved with your settings.
- An intro of five short cards plays the first time you start the game. Any key skips it. "The Last Lamp" is now the subtitle on the main menu.
- The notes carry the whole story: eight more lines, about the shadow, bring it to twenty four. In a calm night those eight are left out. Your place in the story from 0.11.0 is kept.
- Flasks of tea are now always hidden in dead ends, the ones far from the start first.
- The same seed no longer gives the same maze contents as in 0.11.0: the walls are the same, but crystals, notes, levers and puddles moved to make room for the flasks.
- The video behind the main menu is recorded again, so it shows the darker night and the worn walls.
- Four new sounds: the hum of the shadow, the moment it takes you, and the wind and a far bell in the intro.

## 0.11.0 (2026-10-08)

A darker night, and the first changes that come straight from friends who played: sprinting now costs breath, and the map is no longer always in the corner.

### What's new

- Sprinting uses stamina. About six seconds of sprint empties a thin bar under the battery. It fills again after a short rest. Run it empty and you are winded: you breathe hard and cannot sprint until the bar is half full again.
- Flasks of warm tea are hidden in the maze: one on Easy, two on Normal, three on Hard. Walk into one and your stamina is full and sprinting costs nothing for 20 seconds. The bar turns copper while the tea works.
- The map has left the corner of the screen. Hold M and it opens large in the middle. While you hold it you stand still and cannot look around, and the night goes on: the battery keeps draining.
- The notes now tell a story. Sixteen lines left by the lamplighter who walked the maze before you, read in order: each maze has six notes, the ones nearest to the start come first, and the next maze carries on where you stopped. The game remembers your place when you finish a maze.
- Some walls are worn: cracked, mossy or with stones missing. Which ones is decided by the seed, so the same seed gives the same walls. The walls near the start are whole.
- The night is darker. There is less light away from your flashlight, the moon is dimmer, the fog is darker and the flashlight reaches 10 metres, not 16.
- Two new sounds: hard breathing while you are winded, and a cork and a soft note for a flask.
- The round ends with "Through the gate", and a note card is headed "Chalk on the stone".

## 0.10.1 (2026-10-07)

Fullscreen that a screenshot can see.

### What's new

- On Windows, fullscreen is now a borderless window that covers the screen. Screenshots, screen sharing and recordings show the game as it is. Before, they showed one frozen picture.
- On Windows, the fullscreen game no longer minimizes when you switch to another program with Alt+Tab. It stays on its screen, paused, until you come back.
- The fullscreen switch on the settings screen no longer slides from off to on when you open the settings, and it now lights up under the mouse when it is on, too.

## 0.10.0 (2026-10-07)

The maze is no longer silent: the first sounds.

### What's new

- The flashlight clicks when you switch it on and off with F.
- When the battery is empty the switch only gives a dull click, and you hear the same click at the moment the light dies.
- A low battery now warns you with a slow pulse, like a heartbeat. It gets faster as the battery runs down, and it stops when a crystal charges the battery again.
- Picking up a crystal chimes.
- A lever clunks when you pull it, and the gate grinds open when you have collected enough crystals.
- A volume slider on the settings screen, from 0 to 100. It is saved with your other settings, and a short click lets you hear the level while you move it.

## 0.9.0 (2026-10-06)

The game gets a front door: menus, difficulty levels and settings.

### What's new

- The game now starts in a main menu with Play, Settings and Quit. Move with the arrow keys or Tab and choose with Enter.
- Behind the main menu a short video of the maze loops. If the video cannot play, a still picture of the same scene is shown instead.
- Three difficulty levels, Easy, Normal and Hard, chosen in the main menu. A higher level gives you a bigger maze, more crystals to find and a shorter flashlight battery. Easy is the game as it was before.
- You can type a seed in the main menu, or press New seed for another one. The same seed and difficulty always give the same maze.
- Press Escape while you play to pause. From the pause screen you can resume, restart the maze, open the settings or go back to the menu.
- The round now ends with a screen that shows your time, your crystals, the difficulty and the seed. From there you can play the same maze again, start a new maze or go back to the menu.
- A settings screen with mouse sensitivity, field of view, fullscreen and window size. Your choices are saved to a file and are still there the next time you start the game.
- The round pauses by itself when the window loses focus.
- The panels that show information about the game are now one window, with search and pinning.

## 0.8.0 (2026-10-06)

Levers, notes and a sky you can see in the water.

### What's new

- Crystals and puddles now mirror the night sky, so the maze shimmers a little.
- Some walls carry a lever. Look at it and press E (or click) to pull it, and a shortcut wall sinks into the ground.
- Some walls carry a note. Press E to read it: it points you towards the exit, towards the nearest crystal, or just says something.
- A small prompt at the bottom of the screen tells you when you are looking at something you can use.
- Press E or click again to close a note. Walking away from it closes it too.

## 0.7.0 (2026-10-06)

The night gets moody: shadows, glow, fog and a map.

### What's new

- The moon casts shadows across the maze, and so does your flashlight.
- The brightest things, like the crystals, glow softly.
- A low mist hangs over the ground and the edges of the screen darken a little.
- The flashlight is now held in your hand, a bit to the right and below your eyes, so shadows fall in a natural way.
- A small map of the maze sits in the bottom right corner. It only shows the corridors you have already seen, and it is switched on and off with M.

## 0.6.0 (2026-10-05)

A real night sky and uneven ground.

### What's new

- A starry night sky now surrounds the maze.
- The ground is no longer flat: it rises and falls, and the maze, the crystals and you stand on it.
- Grass grows along the bottom of the walls.

## 0.5.0 (2026-10-05)

Now it is a game: collect crystals and escape.

### What's new

- Glowing crystals are hidden in the maze. Walk into one to pick it up.
- Your flashlight battery drains while the flashlight is on, and it flickers when it runs low. Every crystal charges it back a little.
- Collect enough crystals and the gate to the exit sinks into the ground. Then find the exit, which is in the corner farthest from where you start.
- The screen shows your crystals, the battery and the time. When you escape you see your time.
- Press F to switch the flashlight on and off, and R to start again.

## 0.4.0 (2026-10-05)

The maze is lit by the moon and your flashlight.

### What's new

- The maze is dark, lit by the moon and by a flashlight you carry. Press F to switch it on and off.
- Stone walls now show bumps and cracks when the light hits them from the side.
- Flashlight and moonlight are both easy to tell apart: the flashlight makes a bright round spot, the moon gives a soft blue wash.

## 0.3.0 (2026-10-05)

A maze to walk through.

### What's new

- A stone maze with walls, pillars and a floor, built by the game itself.
- You walk inside it on foot with W, A, S and D, look around with the mouse and cannot walk through walls. You slide along them instead.
- Click in the window to grab the mouse, press Escape to give it back, and press Escape again to quit.
- Press N to fly through the walls and look at the maze from outside. Space goes up and Left Shift goes down.
- The panels that show information about the game now use a dark theme, and the key left of 1 hides them.

## 0.1.0 (2026-10-05)

First flight: a cube in the dark.

### What's new

- A cube floats in the window, and you fly around it. Click to grab the mouse, look with the mouse and move with W, A, S and D.
- Space moves you up and Left Shift moves you down.
- Escape gives the mouse back.

## 0.0.0 (2026-10-04)

The very first start: an empty window.

### What's new

- The game opens a window and shows a plain coloured background.
- A small panel shows the frame rate and information about your graphics card.
- Escape closes the window.
