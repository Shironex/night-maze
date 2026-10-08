# The moon shadow removes only moon light, is tested per fragment, and grass casts none
Status: accepted (2026-10-05)
Code: assets/shaders/common/lighting.glsl, assets/shaders/gouraud.frag, assets/shaders/grass.frag

Context: The scene has ambient, moon, flashlight and crystal light, and the shadow map says only whether the moon reaches a point.
Decision: computeLighting returns the moon share separately, and the shader subtracts it times the shadow. Ambient, flashlight and crystals stay lit. Gouraud sums light per vertex but reads the shadow per fragment. Grass receives shadows and casts none.
Why: I rejected multiplying the whole colour by the shadow because it puts out the flashlight where it is needed most. Per vertex shadows depend on mesh density.
Cost and revisit: Blades are thinner than a texel, so their shadow would flicker. Moon intensity rose to 0.2 for shadows and is 0.12 again. I reopen it if grass grows bigger.
