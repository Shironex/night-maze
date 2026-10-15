# Builds the two models of the stile, the place every night begins: a step stile in the
# border wall of the start cell, as dry-stone walls have them.
#   stile       one wall segment with a notch in its top and four flat through-stones
#               that climb to it. It takes the place of the plain segment.
#   stile_post  the oak post beside the steps, with the iron hook the lamp hung on. The
#               hook is empty: the lamp is in the hand of the player. It is not built in
#               this script: its source is a model that Tripo generated,
#               tools/blender/sources/stile_post_tripo.glb, which this script cleans up,
#               squeezes into the footprint of the first post and writes with a picture
#               of its own.
# Output: assets/models/<name>.obj and <name>.mtl for each of the two, and the pictures
# assets/textures/stile_post.png and stile_post_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_stile.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

import bmesh
import bpy

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import build_wall_straight as wall
import generated_common as generated
import make_textures
from mathutils import Matrix, Vector

NAME = "stile"
POST_NAME = "stile_post"

# All sizes are in metres. The geometry is written in Blender space: X is the length of the
# wall, Y is its thickness and Z is the height. After export Z becomes the game Y axis and
# -Y the game +Z axis, so everything at a negative Y stands on the side of the wall that
# faces into the start cell. Both models have the origin of the wall segment: the middle
# of its base. The game draws them with the matrix of that segment.

# ---- the wall with the notch ----
# The notch is 0.7 m wide. Left and right of it the wall keeps its full height.
NOTCH_START = 0.15
NOTCH_END = 0.85
# The body of the wall ends here under the notch, and two coping stones lie on it: the
# wall is 2.4 m high there. The second stone is a little thinner and lower, so the two
# never share a plane with each other or with the coping of the wall.
NOTCH_BODY_TOP = 2.25
NOTCH_STONE_JOINT = 0.52
NOTCH_FIRST_STONE = (0.15, 2.4)  # half thickness, top
NOTCH_SECOND_STONE = (0.14, 2.39)

# ---- the through-stones ----
# Each step: x of its middle, height of its middle, half length along the wall, how far it
# stands out from the middle of the wall, thickness. They climb towards the notch.
STEPS = (
    (-0.52, 0.5, 0.27, 0.42, 0.11),
    (-0.22, 0.95, 0.24, 0.4, 0.1),
    (0.08, 1.4, 0.28, 0.44, 0.12),
    (0.38, 1.85, 0.23, 0.39, 0.1),
)
# A through-stone goes through the whole wall and shows 2 cm on its far face.
STEP_FAR_END = 0.12
# Every stone is turned a little about a point under its middle, so no two lie alike. The
# turns come from a random number generator with this seed.
STEP_SEED = 91
STEP_TURN_CENTRE_Y = -0.26
STEP_MAX_TURN_Z_AXIS = math.radians(6.0) / 2.0  # about the Y axis: one end dips
STEP_MAX_TURN_X_AXIS = math.radians(5.0) / 2.0  # about the X axis: the stone leans

# ---- the post ----
# The post is made from the source by clean_post. The start cell is a corner of the maze,
# so a wall can stand across either end of this segment, and its plinth reaches 0.86 m
# from the middle. The post stands 1 cm clear of that, in front of the far end of the
# notch. Its footprint, at its widest place (the iron bands), is POST_X by POST_Y: the
# place the first post stood in.
POST_X = (0.75, 0.85)
POST_Y = (-0.37, -0.27)
# The post reaches below the ground like the milestone: the wall is lowered to the lowest
# ground under its own footprint, and the post stands 0.3 m in front of it. POST_HEIGHT is
# the height of its top over the ground: above the pillars (3.15 m), like the first post.
POST_BOTTOM = -0.15
POST_HEIGHT = 3.29

SOURCE_FILE = "stile_post_tripo.glb"
# Two corners of the source closer than this (in its own units, about a metre for the whole
# post) are one corner. The file has a corner of its own for every face that meets at a
# seam of the picture, and without this the faces of one surface would not know that they
# are neighbours: not for the smooth normals and not for the islands.
MERGE_DISTANCE = 0.000001
# The pictures of the source are 2048 pixels wide. The post is 3 m tall and seen from a
# metre away at the start of every round, so it gets a quarter of the pixels: every texel
# the average of 2 by 2 of the source.
POST_TEXTURE_SIZE = 1024

# The source is four times as tall as wide, and the game wants a pole 30 times as tall as
# wide. Squeezing everything evenly would stretch the iron bands and the hook into long
# slivers. So the two places with iron, as heights of the source (the lower band and the
# upper band with the hook), keep their own height, and the wood between them takes all of
# the stretch, along its grain. The rest of the model is scaled across by the footprint.
IRON_ZONES = ((0.065, 0.15), (0.74, 0.88))


def add_notched_wall(vertices, faces):
    """Appends the wall: plinth, three pieces of body, the coping and the notch stones."""
    half = wall.HALF_LENGTH
    body = wall.BODY_HALF_THICKNESS
    trim = wall.TRIM_HALF_THICKNESS
    plinth_top = wall.PLINTH_HEIGHT
    coping_bottom = wall.HEIGHT - wall.COPING_HEIGHT

    # Plinth. Its underside lies on the floor and is never seen.
    common.add_box(vertices, faces, (-half, -trim, 0.0), (half, trim, plinth_top), skip=("-z",))
    # The body left of the notch, under it and right of it. Every underside is covered by
    # the plinth and every top by a coping stone. The two tall pieces keep their ends:
    # above the notch they are its two sides.
    common.add_box(
        vertices, faces, (-half, -body, plinth_top), (NOTCH_START, body, coping_bottom),
        skip=("-z", "+z"),
    )
    common.add_box(
        vertices, faces, (NOTCH_START, -body, plinth_top), (NOTCH_END, body, NOTCH_BODY_TOP),
        skip=("-z", "+z"),
    )
    common.add_box(
        vertices, faces, (NOTCH_END, -body, plinth_top), (half, body, coping_bottom),
        skip=("-z", "+z"),
    )
    # The coping left and right of the notch, with all six sides.
    common.add_box(vertices, faces, (-half, -trim, coping_bottom), (NOTCH_START, trim, wall.HEIGHT))
    common.add_box(vertices, faces, (NOTCH_END, -trim, coping_bottom), (half, trim, wall.HEIGHT))
    # The two stones in the notch, with all six sides.
    common.add_box(
        vertices, faces, (NOTCH_START, -NOTCH_FIRST_STONE[0], NOTCH_BODY_TOP),
        (NOTCH_STONE_JOINT, NOTCH_FIRST_STONE[0], NOTCH_FIRST_STONE[1]),
    )
    common.add_box(
        vertices, faces, (NOTCH_STONE_JOINT, -NOTCH_SECOND_STONE[0], NOTCH_BODY_TOP),
        (NOTCH_END, NOTCH_SECOND_STONE[0], NOTCH_SECOND_STONE[1]),
    )


def add_steps(vertices, faces):
    """Appends the four through-stones, each a box turned a little about its own middle."""
    rnd = wall.random_numbers(STEP_SEED)
    for x, z, half_length, out, thickness in STEPS:
        centre = (x, STEP_TURN_CENTRE_Y, z)
        # The order of the rnd() calls is part of the model.
        about_y = (rnd() - 0.5) * 2.0 * STEP_MAX_TURN_Z_AXIS
        about_x = (rnd() - 0.5) * 2.0 * STEP_MAX_TURN_X_AXIS

        first = len(vertices)
        common.add_box(
            vertices, faces, (x - half_length, -out, z - thickness / 2.0),
            (x + half_length, STEP_FAR_END, z + thickness / 2.0),
        )
        for index in range(first, len(vertices)):
            vertices[index] = turned(vertices[index], centre, about_x, about_y)


def turned(point, centre, about_x, about_y):
    """Turns a point about `centre`: first about the X axis, then about the Y axis."""
    x, y, z = wall.sub(point, centre)
    cos_x, sin_x = math.cos(about_x), math.sin(about_x)
    y, z = y * cos_x - z * sin_x, y * sin_x + z * cos_x
    cos_y, sin_y = math.cos(about_y), math.sin(about_y)
    x, z = x * cos_y + z * sin_y, -x * sin_y + z * cos_y
    return wall.add((x, y, z), centre)


def clean_post(model):
    """Returns the post as a bmesh, squeezed into the footprint of POST_X and POST_Y.

    The source is a stubby post, as wide as a quarter of its height, with the hook on its
    +X side. The game wants a pole 3 m tall and 10 cm wide, so the post is scaled along
    its height by a different factor than across. The hook goes to the side of the notch
    (-X), where the first hook was, so the player sees it in profile from the cell.
    """
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or two. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    # The source is one piece already, but the check costs nothing: only the largest island
    # stays.
    post = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in post],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    bmesh.ops.transform(mesh, matrix=Matrix.Rotation(math.pi, 4, "Z"), verts=mesh.verts)
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    low_z = min(corner.co.z for corner in mesh.verts)
    high_z = max(corner.co.z for corner in mesh.verts)
    middle_x = (min(xs) + max(xs)) / 2.0
    middle_y = (min(ys) + max(ys)) / 2.0
    across_x = (POST_X[1] - POST_X[0]) / (max(xs) - min(xs))
    across_y = (POST_Y[1] - POST_Y[0]) / (max(ys) - min(ys))
    # How much each height of the source is stretched: 1 in the zones of iron, and in the
    # wood between them whatever brings the post to its height.
    iron = sum(high - low for low, high in IRON_ZONES)
    along = (POST_HEIGHT - POST_BOTTOM - iron) / (high_z - low_z - iron)

    def stretched(z):
        """The height over POST_BOTTOM of the height z of the source."""
        height = 0.0
        position = low_z
        for low, high in IRON_ZONES + ((high_z, high_z),):
            top = min(z, low)
            if top > position:
                height += (top - position) * along
            top = min(z, high)
            if top > max(low, position):
                height += top - max(low, position)
            position = max(position, high)
        return height

    for corner in mesh.verts:
        corner.co = Vector((
            (POST_X[0] + POST_X[1]) / 2.0 + (corner.co.x - middle_x) * across_x,
            (POST_Y[0] + POST_Y[1]) / 2.0 + (corner.co.y - middle_y) * across_y,
            POST_BOTTOM + stretched(corner.co.z),
        ))
    return mesh


def check_post(mesh):
    """Stops when the post is not where the game (src/game/Stile.hpp) expects it."""
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    zs = [corner.co.z for corner in mesh.verts]
    for found, wanted in ((min(xs), POST_X[0]), (max(xs), POST_X[1]),
                          (min(ys), POST_Y[0]), (max(ys), POST_Y[1]),
                          (min(zs), POST_BOTTOM), (max(zs), POST_HEIGHT)):
        if abs(found - wanted) > 0.0001:
            raise RuntimeError(f"{POST_NAME}: an edge is at {found:.4f}, not {wanted}")
    print(POST_NAME, "has", len(mesh.faces), "triangles")


def write_post_pictures(colour, normal):
    """Writes the colour picture and the normal map of the post at POST_TEXTURE_SIZE."""
    make_textures.save_png(generated.shrink(colour, POST_TEXTURE_SIZE), "stile_post.png")
    generated.save_normal_map(normal, POST_TEXTURE_SIZE, "stile_post_normal.png")


def build(shots):
    common.reset_scene()
    vertices = []
    faces = []
    add_notched_wall(vertices, faces)
    add_steps(vertices, faces)
    model = common.create_mesh_object(NAME, vertices, faces)
    # face_project_uvs: the steps are turned, so their faces are slanted. The density is
    # the one of the walls, so the stone looks as on the plain wall beside it.
    common.face_project_uvs(model.data, common.METRES_PER_UV_UNIT)
    common.assign_textured_material(model, "wall_stone", "wall_stone.png", "wall_stone_normal.png")
    common.export_obj(NAME)
    print(NAME, "has", sum(len(face) - 2 for face in faces), "triangles")
    if shots:
        # From inside the cell, a little to the side, and from the floor in front of the
        # steps, looking up at the notch.
        common.render_review_shots(
            NAME, target=(0.1, 0.0, 1.5), camera_positions=[(-2.4, -5.0, 2.2), (0.6, -2.2, 1.0)]
        )

    source, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean_post(source)
    check_post(mesh)
    write_post_pictures(colour, normal)

    common.reset_scene()
    data = bpy.data.meshes.new(POST_NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The wood is smooth: the exporter writes the normal of every corner as the average of
    # the faces around it, and the bark is made of thousands of small faces.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    post = bpy.data.objects.new(POST_NAME, data)
    bpy.context.scene.collection.objects.link(post)
    common.assign_textured_material(post, "stile_post", "stile_post.png", "stile_post_normal.png")
    common.export_obj(POST_NAME)
    if shots:
        # The whole post, and its top with the band and the hook.
        common.render_review_shots(
            POST_NAME, target=(0.75, -0.32, 2.5),
            camera_positions=[(-0.6, -5.5, 2.4), (-0.2, -1.2, 2.7)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
