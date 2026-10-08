# Regenerates every generated asset of the game in one Blender process: the textures first,
# then the models, then the sky and the heightmap of the terrain.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/make_all.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the other
# scripts next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import build_crystal
import build_flask
import build_gate
import build_lever
import build_note
import build_shade
import build_wall_pillar
import build_wall_straight
import make_heightmap
import make_skybox
import make_textures

shots = "--shots" in sys.argv

# The textures come first: a model script loads its PNG to build the material, and the
# review renders show it.
make_textures.build()

build_wall_straight.build(shots)
build_wall_pillar.build(shots)
build_crystal.build(shots)
build_gate.build(shots)
build_lever.build(shots)
build_note.build(shots)
build_flask.build(shots)
build_shade.build(shots)

# The six faces of the sky. They depend on nothing above: no model uses them.
make_skybox.build()

# The heights of the terrain. Like the sky, it depends on nothing above.
make_heightmap.build()
