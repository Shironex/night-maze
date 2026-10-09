# The crystals are drawn as splinters of the moon, and the dark half of one picture keeps their rind from glowing
Status: accepted (2026-10-09)
Code: tools/blender/build_splinter.py, tools/blender/make_textures.py, src/game/GameplayRenderer.cpp, tests/ObjLoaderTests.cpp

Context: The story calls the things the player collects splinters of the moon, and the models were six-sided quartz. A sliver with a strip of grey rind needs two surfaces, one that glows and one that does not, and the shaders have one glow value for a whole model.
Decision: Two models, one sliver and three, built ring by ring from a fixed seed. One picture has two halves, like the lantern: the rind on the left is dark and the fracture on the right is pale. The glow is the colour times the glow value, so the fracture shines and the rind stays stone. Only the look changes: the code, the settings and the texts still say crystal.
Why: I rejected a second material with its own glow because a model has exactly one material, and a glow picture because that is a new sampler in four shaders for one model.
Cost and revisit: The rind is darker than moon dust should be, and in the light of the splinter itself it is tinted turquoise: grey shows only in the flashlight. I reopen it if another model needs a part that glows and a part that must stay light.
