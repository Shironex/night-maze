# Builds the two models of the shade: the tall hooded figure that follows the player through
# the maze whenever the flashlight is not on it. Unlike every other model here, the figure
# is not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/shade_tripo.glb: a hooded figure in a long robe, with a dark
# mannequin's face in the hood and dark mannequin's hands in the sleeves, one material with
# a bright blue colour picture and a normal map, about 0.98 m tall. This script cleans it
# up and writes the game files, the same bytes on every run.
#
# shade is the cloth: the robe without the hands, scaled to the height of the shade.
# shade_hollow is what the cloth is wrapped around: a curved sheet in front of the face in
# the hood, and lids over the places in the sleeves where the hands were. The game draws
# those faces as a piece of the night sky and not as cloth.
# Output: assets/models/shade.obj, shade.mtl, shade_hollow.obj and shade_hollow.mtl, and
# the pictures assets/textures/shade.png and shade_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_shade.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import make_textures

NAME = "shade"
HOLLOW_NAME = "shade_hollow"

SOURCE_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sources",
                           "shade_tripo.glb")

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis, and the front of the figure (Blender -Y) looks
# along the game +Z axis: the game turns the model around the vertical axis from there.
# The source already stands on Blender Z = 0 and looks along Blender -Y.
# The figure is 2.1 m tall, taller than the player (1.8 m), and its origin is on the
# ground in the middle of its base.
HEIGHT = 2.1

# The robe goes this far below the origin. The source ends on the ground in a pool of
# folds. The ground of the maze is uneven (up to about a tenth of a metre per metre), and
# the walk of the shade lifts it by up to 4 cm: a hem that ended exactly at the origin
# would hang in the air on the low side. So the figure is scaled to HEIGHT + HEM_DEPTH and
# lowered by HEM_DEPTH. The flat pool of the folds lies under the ground then.
HEM_DEPTH = 0.1

# Two corners of the source closer than this (in its own units, about a metre for the
# whole figure) are one corner. The file has a corner of its own for every face that
# meets at a seam of the picture, and without this the faces of one surface would not
# know that they are neighbours: not for the smooth normals and not for the islands.
MERGE_DISTANCE = 0.000001

# The mannequin. Its face and its hands are nearly black and grey in the picture, and the
# robe is blue, so a face of the model belongs to the mannequin when the texel at the
# middle of its UVs is this dark (all three channels below MANNEQUIN_DARK, sRGB) and this
# grey (the channels at most MANNEQUIN_GREY apart). Below MANNEQUIN_HANDS_SHARE of the
# height those are the hands, above it the face.
MANNEQUIN_DARK = 0.08
MANNEQUIN_GREY = 0.05
MANNEQUIN_HANDS_SHARE = 0.65

# The pictures of the source are 2048 pixels wide. The game gets them at a quarter of the
# pixels, every texel the average of 2 by 2 of the source: the figure is seen from a metre
# away at the closest, and two pictures of 2048 would be 32 MB on the graphics card for
# one figure.
TEXTURE_SIZE = 1024

# The colour. The source is a bright royal blue. The game wants a blue so dark that it is
# nearly black: a black picture would show a hole in the beam of the flashlight and not a
# shape. The brightest channel of a texel of the source (sRGB) becomes a brightness, and
# the brightness a dark blue, so the folds and the seams the source painted stay. The
# numbers are chosen in the game: the flashlight is warm, and a bluish grey in the
# picture reads as plain grey in its beam.
COLOUR_FLOOR = 0.05
COLOUR_RANGE = 0.34
COLOUR_TINT = (0.18, 0.24, 0.70)

# The night in the hood: a curved sheet in front of the face of the mannequin. Seen from
# the front, the hood frames the face tightly, so the sheet takes its outline from the face:
# rays go into the hood along Blender +Y every NIGHT_RAY_STEP across and every
# NIGHT_ROW_STEP up, and every row of the sheet reaches from the leftmost to the
# rightmost ray that meets the mannequin first, NIGHT_MARGIN further on both sides, with
# NIGHT_COLUMNS faces across. One row more at the bottom and at the top reaches
# NIGHT_MARGIN beyond the face. The margin lies behind the brim of the hood, so the edge of
# the night is the edge of the cloth.
NIGHT_RAY_STEP = 0.005
NIGHT_ROW_STEP = 0.01
NIGHT_MARGIN = 0.01
NIGHT_COLUMNS = 12
# The sheet lies NIGHT_GAP in front of the tip of the nose, like a chalk mark in front of
# a wall, and bends back towards its sides, NIGHT_BEND times the square of the distance
# from the middle (in metres): the face is narrow and the sides of the brim are deeper than
# the nose, and a flat sheet would stand in front of them.
NIGHT_GAP = 0.004
NIGHT_BEND = 6.0

# The places of the hands are closed by faces that follow the edge of each hole, looking
# both ways. Like the sheet in the hood they belong to shade_hollow. Their piece of the
# picture is a tiny patch of the mannequin's face, which is black: this many UV units per
# metre around the UV middle of the darkest face of the mannequin.
DARK_UV_UNITS_PER_METRE = 0.01


def import_source():
    """Imports the GLB and returns its mesh object and its two pictures as arrays.

    The pictures are rows from the bottom, as Blender stores them, with the channels from
    0 to 1 exactly as the file has them (sRGB for the colour, the directions for the
    normal map).
    """
    common.reset_scene()
    bpy.ops.import_scene.gltf(filepath=SOURCE_FILE)
    model = next(item for item in bpy.data.objects if item.type == "MESH")
    nodes = model.data.materials[0].node_tree.nodes
    surface = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    colour = surface.inputs["Base Color"].links[0].from_node.image
    normal_map = next(node for node in nodes if node.type == "NORMAL_MAP")
    normal = normal_map.inputs["Color"].links[0].from_node.image
    return model, picture_array(colour), picture_array(normal)


def picture_array(image):
    width, height = image.size
    pixels = np.empty(width * height * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    return pixels.reshape(height, width, 4)[..., :3]


def texel_at(picture, face, uv_layer):
    """The colour of the picture at the middle of the UVs of a face."""
    height, width, _ = picture.shape
    u = sum(loop[uv_layer].uv.x for loop in face.loops) / len(face.loops)
    v = sum(loop[uv_layer].uv.y for loop in face.loops) / len(face.loops)
    column = min(width - 1, max(0, int(u * width)))
    row = min(height - 1, max(0, int(v * height)))
    return picture[row, column]


def is_mannequin(texel):
    return texel.max() < MANNEQUIN_DARK and texel.max() - texel.min() < MANNEQUIN_GREY


def neighbours(face):
    return [other for edge in face.edges for other in edge.link_faces if other is not face]


def largest_island(mesh):
    """The faces of the largest group of faces that share edges with each other."""
    mesh.faces.index_update()
    seen = set()
    largest = []
    for first in mesh.faces:
        if first.index in seen:
            continue
        island = [first]
        seen.add(first.index)
        # The island grows face by face. The list itself is the queue.
        for face in island:
            for other in neighbours(face):
                if other.index not in seen:
                    seen.add(other.index)
                    island.append(other)
        if len(island) > len(largest):
            largest = island
    return largest


def boundary_loops(mesh):
    """The holes of the surface: every closed chain of edges with a face on one side only.

    Each hole is a list of its corners in order, starting at the edge with the lowest
    index, so the same mesh gives the same lists on every run. Two holes may touch at one
    corner. So the walk does not go from corner to corner but from face to face: the next
    edge of a hole is found by turning around the corner through the faces there, until
    an edge with a face on one side only comes.
    """
    mesh.edges.index_update()
    loops = []
    done = set()
    for start in [edge for edge in mesh.edges if edge.is_boundary]:
        if start.index in done:
            continue
        chain = []
        # The corner of the only face of the edge that lies on it, in the order of the face.
        loop = start.link_loops[0]
        while loop.edge.index not in done:
            done.add(loop.edge.index)
            chain.append(loop.vert)
            loop = loop.link_loop_next
            while not loop.edge.is_boundary:
                loop = loop.link_loop_radial_next.link_loop_next
        loops.append(chain)
    return loops


def give_dark_uvs(face, middle, uv_layer, dark_uv):
    """Gives a face a tiny piece of the picture around dark_uv.

    The face is projected onto its own plane, as common.face_project_uvs does: `up` is the
    height as far as the face allows and `right` points to the right for someone who looks
    at the face from outside. So the piece is never a mirror image. `middle` is the point
    that lands on dark_uv.
    """
    face.normal_update()
    up = Vector((0.0, 0.0, 1.0)) - face.normal * face.normal.z
    if up.length < 0.000001:
        up = Vector((0.0, 1.0, 0.0))
    up.normalize()
    right = up.cross(face.normal)
    for loop in face.loops:
        offset = loop.vert.co - middle
        loop[uv_layer].uv = (dark_uv[0] + offset.dot(right) * DARK_UV_UNITS_PER_METRE,
                             dark_uv[1] + offset.dot(up) * DARK_UV_UNITS_PER_METRE)


def close_hole(mesh, hole, uv_layer, dark_uv):
    """Closes one hole with faces along its edge, once looking each way. Returns them.

    Each side has corners of its own: Blender keeps only one of two faces that share all
    their corners. The hole is first one face with all its corners, then cut into
    triangles that stay inside its outline, also where the outline bends inwards.
    """
    middle = sum((corner.co for corner in hole), Vector()) / len(hole)
    closed = []
    for ring in (hole, hole[::-1]):
        lid = mesh.faces.new([mesh.verts.new(corner.co) for corner in ring])
        triangles = bmesh.ops.triangulate(mesh, faces=[lid], quad_method="BEAUTY",
                                          ngon_method="BEAUTY")["faces"]
        for face in triangles:
            give_dark_uvs(face, middle, uv_layer, dark_uv)
        closed.extend(triangles)
    return closed


def mannequin_rows(mesh, mannequin):
    """Finds the face of the mannequin as the rays from the front see it.

    Returns one (height, left, right) per row of NIGHT_ROW_STEP in which a ray meets the
    mannequin before any cloth, from the bottom up, and the y of the tip of the nose.
    """
    tree = BVHTree.FromBMesh(mesh)
    mesh.faces.ensure_lookup_table()
    heights = [face.calc_center_median().z for face in mannequin]
    reach = max(abs(corner.co.x) for face in mannequin for corner in face.verts)
    columns = round(reach / NIGHT_RAY_STEP)
    rows = []
    nose = 0.0
    row = round(min(heights) / NIGHT_ROW_STEP)
    while row * NIGHT_ROW_STEP <= max(heights):
        height = row * NIGHT_ROW_STEP
        seen = []
        for column in range(-columns, columns + 1):
            x = column * NIGHT_RAY_STEP
            point, _, number, _ = tree.ray_cast(Vector((x, -1.0, height)),
                                                Vector((0.0, 1.0, 0.0)))
            if point is not None and mesh.faces[number] in mannequin:
                seen.append(x)
                nose = min(nose, point.y)
        if seen:
            rows.append((height, min(seen), max(seen)))
        row += 1
    return rows, nose


def night_depth(x, nose):
    """The y of the sheet of night at x: in front of the nose, bending back to the sides."""
    return nose - NIGHT_GAP + NIGHT_BEND * x * x


def add_night(mesh, mannequin, uv_layer, dark_uv):
    """Adds the sheet of night in front of the face of the mannequin.

    Returns its faces and its middle.
    """
    rows, nose = mannequin_rows(mesh, mannequin)
    # The outline with its margin, and one row more under and over the face.
    outline = [(height, left - NIGHT_MARGIN, right + NIGHT_MARGIN)
               for height, left, right in rows]
    outline.insert(0, (rows[0][0] - NIGHT_MARGIN,) + outline[0][1:])
    outline.append((rows[-1][0] + NIGHT_MARGIN,) + outline[-1][1:])

    # A grid of corners, row by row from the bottom, from left to right in every row.
    grid = []
    for height, left, right in outline:
        corners = []
        for column in range(NIGHT_COLUMNS + 1):
            x = left + (right - left) * column / NIGHT_COLUMNS
            corners.append(mesh.verts.new((x, night_depth(x, nose), height)))
        grid.append(corners)

    middle = Vector((0.0, night_depth(0.0, nose), (outline[0][0] + outline[-1][0]) / 2.0))
    sheet = []
    for lower, upper in zip(grid, grid[1:]):
        for column in range(NIGHT_COLUMNS):
            # Counter clockwise as seen from the front, where +X is to the right.
            face = mesh.faces.new((lower[column], lower[column + 1], upper[column + 1],
                                   upper[column]))
            give_dark_uvs(face, middle, uv_layer, dark_uv)
            sheet.append(face)
    return sheet, middle


def clean(model, colour):
    """Returns the cleaned surface as a bmesh, the set of its hollow faces and the middle
    of the night in the hood."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a millimetre or two. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    uv_layer = mesh.loops.layers.uv.active
    top = max(corner.co.z for corner in mesh.verts)

    # The hands come out. What is left of the sleeves is open where they were.
    mannequin = [face for face in mesh.faces if is_mannequin(texel_at(colour, face, uv_layer))]
    hands = [face for face in mannequin
             if face.calc_center_median().z < MANNEQUIN_HANDS_SHARE * top]
    hand_corners = {tuple(corner.co) for face in hands for corner in face.verts}
    bmesh.ops.delete(mesh, geom=hands, context="FACES")

    # Taking out the hands leaves bits of them that hang on nothing, and the source has
    # specks of a few millimetres of its own. Only the largest island stays: the figure.
    figure = set(largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in figure],
                     context="FACES")
    mesh.faces.index_update()
    mesh.verts.index_update()

    # The holes the hands left. The source has a few small holes of its own, in the pool
    # of the hem under the ground and at the back of the hood, and they stay open: the
    # hole of a hand is one with a corner that was a corner of the hand. A hole of two
    # edges is a slit that has nothing to close.
    hand_holes = [hole for hole in boundary_loops(mesh) if len(hole) > 2 and
                  any(tuple(corner.co) in hand_corners for corner in hole)]

    # The size of the game: the hem HEM_DEPTH under the ground and the tip of the hood
    # HEIGHT over it, the middle of the base on the origin.
    scale = (HEIGHT + HEM_DEPTH) / top
    low = Vector((min(corner.co.x for corner in mesh.verts),
                  min(corner.co.y for corner in mesh.verts), 0.0))
    high = Vector((max(corner.co.x for corner in mesh.verts),
                   max(corner.co.y for corner in mesh.verts), 0.0))
    middle = (low + high) / 2.0
    for corner in mesh.verts:
        corner.co = (corner.co - middle) * scale - Vector((0.0, 0.0, HEM_DEPTH))

    # The face of the mannequin stays in the cloth, black, behind the night. Its darkest
    # face lends its black to the night where the game shows no sky.
    face = {each for each in mesh.faces if is_mannequin(texel_at(colour, each, uv_layer))}
    darkest = min(sorted(face, key=lambda each: each.index),
                  key=lambda each: texel_at(colour, each, uv_layer).max())
    dark_uv = (sum(loop[uv_layer].uv.x for loop in darkest.loops) / len(darkest.loops),
               sum(loop[uv_layer].uv.y for loop in darkest.loops) / len(darkest.loops))

    sheet, night_middle = add_night(mesh, face, uv_layer, dark_uv)
    hollow = set(sheet)
    for hole in hand_holes:
        hollow.update(close_hole(mesh, hole, uv_layer, dark_uv))
    return mesh, hollow, night_middle


def write_pictures(colour, normal):
    """Writes the colour picture, made dark blue, and the normal map at TEXTURE_SIZE."""
    shrink = colour.shape[0] // TEXTURE_SIZE

    def average(picture):
        # Every texel is the average of a square of shrink by shrink texels of the source.
        return picture.reshape(TEXTURE_SIZE, shrink, TEXTURE_SIZE, shrink, 3).mean(axis=(1, 3))

    brightness = COLOUR_FLOOR + COLOUR_RANGE * average(colour).max(axis=-1)
    make_textures.save_png(brightness[..., None] * np.array(COLOUR_TINT), "shade.png")

    # The directions are averaged as directions: from 0..1 to -1..1, then to length 1 again.
    directions = average(normal) * 2.0 - 1.0
    directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    make_textures.save_png(directions * 0.5 + 0.5, "shade_normal.png")


def export(name, mesh, keep, smooth, shots):
    """Writes the faces of `mesh` in the set `keep` as one textured model."""
    common.reset_scene()
    part = mesh.copy()
    part.faces.ensure_lookup_table()
    # copy() keeps the order of the faces, so the faces of the copy are found by index.
    numbers = {face.index for face in keep}
    bmesh.ops.delete(part, geom=[face for face in part.faces if face.index not in numbers],
                     context="FACES")
    bmesh.ops.delete(part, geom=[corner for corner in part.verts if not corner.link_faces],
                     context="VERTS")

    data = bpy.data.meshes.new(name)
    part.to_mesh(data)
    part.free()
    # The cloth is smooth: the exporter writes the normal of every corner as the average
    # of the faces around it. It has soft folds made of thousands of small faces, and one
    # normal per face would show every one of them. The hollow is flat, one normal per
    # face, as in the other models: it shows the sky, which does not depend on the
    # normal, and a smooth normal over the bent lids of the sleeves would lean the wrong
    # way on a few faces and turn their picture over for the normal map.
    if smooth:
        data.shade_smooth()
    else:
        data.shade_flat()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    model = bpy.data.objects.new(name, data)
    bpy.context.scene.collection.objects.link(model)

    # Both models wear the picture of the cloth. The game shows the sky on the hollow and
    # falls back to this picture, which is black there, when it draws no sky.
    common.assign_textured_material(model, "shade", "shade.png", "shade_normal.png")
    common.export_obj(name)

    if shots:
        # Three views aimed at the chest: from the front, from the side and from where
        # a player stands who looks up at it.
        common.render_review_shots(
            name,
            target=(0.0, 0.0, 1.05),
            camera_positions=[(0.0, -5.6, 1.4), (4.6, -3.0, 1.7), (1.2, -3.4, 1.7)],
        )


def build(shots):
    model, colour, normal = import_source()
    mesh, hollow, centre = clean(model, colour)
    mesh.faces.index_update()
    write_pictures(colour, normal)

    # The middle of the night in the hood, which the game spreads the stars around
    # (SHADE_HOOD_CENTRE in src/game/NightMazeAppPasses.cpp), in game coordinates.
    print(f"The night in the hood: middle at ({centre.x:.3f}, {centre.z:.3f}, {-centre.y:.3f})")

    export(NAME, mesh, set(mesh.faces) - hollow, True, shots)
    export(HOLLOW_NAME, mesh, hollow, False, shots)
    mesh.free()


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
