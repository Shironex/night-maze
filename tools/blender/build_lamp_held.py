# Builds the lamp the player holds: a dented tin railway hand lamp with a round lens in
# the side of its body, a brass band, a vented cap and a wire bail on top. The body is not
# built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/lamp_held_tripo.glb: the lamp standing upright with its lens
# looking along Blender -Y, about 0.98 units tall, one material with a colour picture and a
# normal map of 2048 by 2048. This script lays it along the beam, scales it, thins it out
# and writes the game files, the same bytes on every run.
#
# It comes as three models that fit into each other, because the game gives each a glow
# of its own: lamp_held is the lamp from the source, lamp_held_lens a flat disc just in
# front of its glass and lamp_held_window a ring of flat panes inside the vents of the
# cap, where the light of the splinter that feeds the lamp shows. The two glow pieces are
# built here. All three wear lamp_held.png.
# Output: assets/models/lamp_held.obj, lamp_held_lens.obj, lamp_held_window.obj and their
# .mtl files, and the pictures assets/textures/lamp_held.png and lamp_held_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_lamp_held.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "lamp_held"
LENS_NAME = "lamp_held_lens"
WINDOW_NAME = "lamp_held_window"

SOURCE_FILE = "lamp_held_tripo.glb"

# All sizes are in metres unless they say "source units". The geometry is written in
# Blender space, where Z is the height. The lamp lies along Blender +Y, which is -Z in the
# game: the way the player looks. The lens is on the side of the body, so the lamp stands
# upright with its lens looking down +Y, the way a railway lamp is carried by its bail.
# Its origin is the middle of its butt: the point on the axis of the lens where it leaves
# the back of the lamp, the plane of the rearmost corner. The game puts the lamp on the
# axis of its beam (heldLampMatrix in src/game/Lighting.cpp) and turns it about that axis
# only, so the beam leaves through the middle of the lens.
#
# The lamp is HEIGHT tall from the foot of its body to the top of its bail. A real lamp of
# this kind is nearer 0.3 m, but this one hangs 0.35 m in front of the hand, and the old
# flashlight was 0.224 m long and 0.067 m wide: at 0.17 m the lamp reaches about 0.11 m
# along the beam and covers the lower right corner of the picture up to about where the
# head of the flashlight did. Seen from the eye with the settings of the game, it covers
# 2.1 percent of the picture (the flashlight 1.2 percent), and its top is 23 percent of
# the height of the picture above the lower edge (the flashlight 22 percent).
HEIGHT = 0.17

# The height of the source from the foot of its body to the top of its bail, in source
# units. The script stops if the source is not this tall, so SCALE is the one checked.
SOURCE_TOP = 0.9788
SCALE = HEIGHT / SOURCE_TOP

# The lens of the source, measured once in source units: the middle of its round glass
# (x and z, the glass looks along -Y), the radius of the glass inside the brass bezel and
# how far forward the dome of the glass reaches (the most negative y of the glass). The
# bezel reaches 0.320 forward, further out than the glass, so the disc of the lens stops
# at the edge of the glass.
LENS_CENTRE = (0.0021, 0.3740)
LENS_GLASS_RADIUS = 0.155
LENS_DOME_FRONT = -0.3237
# The disc of the lens lies LENS_OUT metres from the butt along the axis, a hair in front
# of the dome of the glass, like a chalk mark in front of a wall: the script stops if it
# is not between LENS_GAP and 1 mm in front of it. The game hangs the glow of the beam on
# it (HELD_LAMP_LENS_OUT in src/game/Lighting.hpp is LENS_OUT).
LENS_OUT = 0.113
LENS_GAP = 0.0003
LENS_SIDES = 16

# The window: a small pane on the back of the shoulder of the body, the side the player
# sees, like the window of a gauge, WINDOW_WIDTH wide and centred on the axis of the lens
# seen from behind. The shoulder is the highest tin under the cap, and the window must be
# that high: the lower edge of the picture cuts the body of the lamp about 2.5 cm above
# the axis, and the lower edge of the pane is at 4.2 cm, about 5 percent of the picture
# above that edge. Its lower edge, its middle and its upper edge are aimed at WINDOW_ROWS,
# (y, z) in metres,
# and found by rays along WINDOW_RAY, the way the eye of the player looks at the lamp
# (from behind and above), so the pane lies on the tin the player sees and not on the
# bail or under the brim of the cap. The shoulder is round and dented, so the pane is a
# grid of WINDOW_COLUMNS by two flat faces that follows it, each corner WINDOW_GAP in
# front of the tin: with less, the dents of the tin reach through the pane. Two earlier
# places did not work: a ring in the vents of the cap glowed like a pair of eyes, and a
# pane on the back of the body was below the lower edge of the picture.
WINDOW_WIDTH = 0.03
WINDOW_ROWS = ((0.0135, 0.0422), (0.0195, 0.0464), (0.0255, 0.0497))
WINDOW_RAY = (0.0, 0.85, -0.53)
WINDOW_COLUMNS = 6
WINDOW_GAP = 0.0015
# A ray that meets the lamp further than this from the point it aims at has met something
# else (the bail, the cap), and the script stops.
WINDOW_MISS = 0.003

# Two corners of the source closer than this (in its own units, about a metre for the
# whole lamp) are one corner. The file has a corner of its own for every face that meets
# at a seam of the picture, and without this the smooth normals would break at every seam.
MERGE_DISTANCE = 0.000001

# The source has 17,437 triangles. The lamp is never more than a hand wide on the screen,
# so the Decimate modifier (collapse, which gives the same result on every run) keeps this
# share of them.
DECIMATE_RATIO = 0.5

# The pictures of the source are 2048 pixels wide. The game gets them at 1024, every texel
# the average of 2 by 2 of the source: the lamp is the closest thing the player ever sees.
TEXTURE_SIZE = 1024

# The glow pieces need plain pale colours, because the glow of a surface is its colour
# times the glow value of the draw: the lens a warm white, the window the pale teal of the
# splinter. Neither reaches 1, so the glow has room above them. They are painted into two
# patches of lamp_held.png that no face of the source uses (the script stops if one does),
# with a flat normal map there. Each patch is (left, bottom, right, top) in pixels of the
# 1024 picture, and a face of a glow piece takes a small part of the middle of its patch.
LENS_COLOUR = (0.94, 0.90, 0.80)
WINDOW_COLOUR = (0.61, 0.91, 0.87)
LENS_PATCH = (480, 992, 528, 1024)
WINDOW_PATCH = (608, 1008, 656, 1024)
PIECE_UV_UNITS_PER_METRE = 0.1


def patch_centre(patch):
    left, bottom, right, top = patch
    return ((left + right) / 2.0 / TEXTURE_SIZE, (bottom + top) / 2.0 / TEXTURE_SIZE)


def check_patches_free(mesh, uv_layer):
    """Stops the script if a face of the source uses a texel of a glow patch."""
    for face in mesh.faces:
        us = [loop[uv_layer].uv.x * TEXTURE_SIZE for loop in face.loops]
        vs = [loop[uv_layer].uv.y * TEXTURE_SIZE for loop in face.loops]
        for left, bottom, right, top in (LENS_PATCH, WINDOW_PATCH):
            if min(us) < right and max(us) > left and min(vs) < top and max(vs) > bottom:
                raise ValueError(f"a face of the source uses the patch {(left, bottom, right, top)}")


def placement(source_back):
    """The matrix that takes the source to the game: turned half around Z so the lens
    looks along +Y, the butt moved to the origin and scaled to HEIGHT.

    source_back is the largest y of the source, its rearmost plane before the turn.
    """
    butt = Vector((LENS_CENTRE[0], source_back, LENS_CENTRE[1]))
    return (Matrix.Scale(SCALE, 4) @ Matrix.Rotation(math.pi, 4, "Z") @
            Matrix.Translation(-butt))


def clean(model):
    """Returns the surface of the source as a bmesh in the place of the game."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or two. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    check_patches_free(mesh, mesh.loops.layers.uv.active)

    # Every island stays: besides the lamp there are about thirty small ones, and they are
    # the rivets, the brass knobs and the hinge of the lens door, which sit on its surface.
    low = min(corner.co.z for corner in mesh.verts)
    top = max(corner.co.z for corner in mesh.verts)
    if abs(low) > 0.000001 or abs(top - SOURCE_TOP) > 0.0001:
        raise ValueError(f"the source reaches from {low} to {top}, not from 0 to {SOURCE_TOP}")
    back = max(corner.co.y for corner in mesh.verts)
    bmesh.ops.transform(mesh, matrix=placement(back), verts=mesh.verts)
    # The turn takes a y of the source to back - y, so the dome comes out here.
    dome = (back - LENS_DOME_FRONT) * SCALE
    if not LENS_GAP <= LENS_OUT - dome <= 0.001:
        raise ValueError(f"the disc of the lens is {LENS_OUT - dome} m in front of the glass")
    return mesh


def write_pictures(colour, normal):
    """Writes the colour picture and the normal map at TEXTURE_SIZE, with the two patches
    of the glow pieces painted in."""
    picture = generated.shrink(colour, TEXTURE_SIZE)
    directions = generated.shrink(normal, TEXTURE_SIZE) * 2.0 - 1.0
    directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    for (left, bottom, right, top), paint in ((LENS_PATCH, LENS_COLOUR),
                                              (WINDOW_PATCH, WINDOW_COLOUR)):
        # Rows from the bottom, as v counts them.
        picture[bottom:top, left:right] = paint
        directions[bottom:top, left:right] = (0.0, 0.0, 1.0)
    make_textures.save_png(picture, "lamp_held.png")
    make_textures.save_png(directions * 0.5 + 0.5, "lamp_held_normal.png")


def lens_piece():
    """The corners and faces of the lens: a fan of LENS_SIDES triangles that looks along
    +Y, LENS_OUT from the butt."""
    radius = LENS_GLASS_RADIUS * SCALE
    corners = [(0.0, LENS_OUT, 0.0)]
    for corner in range(LENS_SIDES):
        angle = corner / LENS_SIDES * 2.0 * math.pi
        corners.append((radius * math.cos(angle), LENS_OUT, radius * math.sin(angle)))
    # Counter clockwise as seen from +Y, where +X is to the left and +Z up.
    faces = [(0, 1 + (corner + 1) % LENS_SIDES, 1 + corner) for corner in range(LENS_SIDES)]
    return corners, faces


def window_piece(body):
    """The corners and faces of the window: a grid on the back of the shoulder, looking
    back and up, towards the player. Each corner is found by a ray along WINDOW_RAY onto
    the thinned body."""
    tree = BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get())
    ray = Vector(WINDOW_RAY).normalized()
    corners = []
    for column in range(WINDOW_COLUMNS + 1):
        x = -WINDOW_WIDTH / 2.0 + WINDOW_WIDTH * column / WINDOW_COLUMNS
        for y, z in WINDOW_ROWS:
            aim = Vector((x, y, z))
            hit = tree.ray_cast(aim - ray, ray)[0]
            if hit is None or (hit - aim).length > WINDOW_MISS:
                raise ValueError(f"the ray to the window at {tuple(aim)} misses the shoulder")
            corners.append(tuple(hit - ray * WINDOW_GAP))
    # Counter clockwise as seen from behind and above, where +X is to the right.
    # Corner number row + rows * column, rows from the lower edge up.
    rows = len(WINDOW_ROWS)
    faces = [(rows * column + row, rows * (column + 1) + row, rows * (column + 1) + row + 1,
              rows * column + row + 1)
             for column in range(WINDOW_COLUMNS) for row in range(rows - 1)]
    return corners, faces


def export_piece(name, corners, faces, patch, shots_scene=False):
    """Writes one glow piece: flat faces with a small part of the middle of its patch."""
    if not shots_scene:
        common.reset_scene()
    model = common.create_mesh_object(name, corners, faces)
    common.piece_project_uvs(model.data, [patch_centre(patch)] * len(faces),
                             PIECE_UV_UNITS_PER_METRE)
    common.assign_textured_material(model, NAME, "lamp_held.png", "lamp_held_normal.png")
    if not shots_scene:
        common.export_obj(name)


def add_body(mesh):
    """Puts the lamp into the scene as an object, thinned out by the Decimate modifier."""
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    # The tin is smooth: the exporter writes the normal of every corner as the average of
    # the faces around it, as for the cloth of the shade. The source is thousands of small
    # faces, and one normal per face would show every one of them.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    model = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(model)
    thin = model.modifiers.new("decimate", "DECIMATE")
    thin.decimate_type = "COLLAPSE"
    thin.ratio = DECIMATE_RATIO
    # The thinning leaves a few edges without a face, which the exporter would write as
    # lines. So it is applied here, and those edges go.
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    thinned = bmesh.new()
    thinned.from_mesh(evaluated.to_mesh())
    evaluated.to_mesh_clear()
    model.modifiers.remove(thin)
    bmesh.ops.delete(thinned, geom=[edge for edge in thinned.edges if edge.is_wire],
                     context="EDGES")
    bmesh.ops.delete(thinned, geom=[corner for corner in thinned.verts if not corner.link_edges],
                     context="VERTS")
    thinned.to_mesh(data)
    thinned.free()
    common.assign_textured_material(model, NAME, "lamp_held.png", "lamp_held_normal.png")
    return model


def check(model):
    """Stops the script if the lamp breaks one of the rules the game relies on, and prints
    its size."""
    evaluated = model.evaluated_get(bpy.context.evaluated_depsgraph_get())
    data = evaluated.to_mesh()
    points = np.array([corner.co[:] for corner in data.vertices])
    triangles = sum(len(polygon.vertices) - 2 for polygon in data.polygons)
    low, high = points.min(axis=0), points.max(axis=0)
    radius = LENS_GLASS_RADIUS * SCALE
    # The butt is the back of the lamp, and no corner in front of the glass would cover
    # the disc of the lens.
    if low[1] < -0.000001:
        raise ValueError(f"a corner lies {-low[1]} m behind the butt")
    inside = np.hypot(points[:, 0], points[:, 2]) < radius
    if points[inside, 1].max() >= LENS_OUT:
        raise ValueError("a corner in front of the glass reaches through the disc of the lens")
    if abs(high[2] - low[2] - HEIGHT) > 0.0001:
        raise ValueError(f"the lamp is {high[2] - low[2]} m tall, not {HEIGHT} m")
    print(f"lamp_held: {triangles} triangles, from {low.round(4).tolist()} to "
          f"{high.round(4).tolist()} in Blender space")
    evaluated.to_mesh_clear()


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model)
    write_pictures(colour, normal)

    common.reset_scene()
    body = add_body(mesh)
    check(body)
    common.export_obj(NAME)

    window = window_piece(body)
    export_piece(LENS_NAME, *lens_piece(), LENS_PATCH)
    export_piece(WINDOW_NAME, *window, WINDOW_PATCH)
    print(f"lamp_held_window: corners {[tuple(round(c, 4) for c in corner) for corner in window[0]]}")

    if shots:
        # The three models together, as the game draws them. Four views aimed at the
        # middle of the lamp: from the eye of the player (behind, to the left and above,
        # with the settings of the game), from the front, into the lens, from the side,
        # and close behind the body, at the window.
        common.reset_scene()
        add_body(mesh)
        export_piece(LENS_NAME, *lens_piece(), LENS_PATCH, shots_scene=True)
        export_piece(WINDOW_NAME, *window, WINDOW_PATCH, shots_scene=True)
        common.render_review_shots(
            NAME,
            target=(0.0, 0.056, 0.015),
            camera_positions=[(-0.199, -0.3245, 0.2495), (0.10, 0.38, 0.07),
                              (0.36, 0.056, 0.07), (-0.04, -0.16, 0.03)],
        )
    mesh.free()


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
