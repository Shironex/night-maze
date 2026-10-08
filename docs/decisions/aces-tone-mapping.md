# ACES is the default tone mapping curve, with Reinhard and None as switches
Status: accepted (2026-10-05)
Code: assets/shaders/post/composite.frag, src/game/PostProcess.hpp, src/game/Skybox.hpp

Context: The HDR buffer holds values above 1, such as crystal glow and the moon, and the screen shows 0 to 1.
Decision: The composite shader has None, Reinhard and Aces (the Narkowicz fit). Aces is the default at exposure 1.0, and it still is. Sky brightness is 2.2 to offset the curve.
Why: A night scene is mostly dark with a few things that must be clearly bright, which an S curve does. I rejected Reinhard as the default because it puts scene white at one half and flattens everything. The switch shows the same buffer three ways.
Cost and revisit: Light values are tuned to this curve, and bright colours lose saturation. I reopen it if the night is unreadable on another screen, changing exposure first.
