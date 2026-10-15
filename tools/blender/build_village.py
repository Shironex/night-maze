# Builds the village on the ridge, and five small models of light: the windows that are lit
# after each night of the campaign. The cottages are not built in this script. Their source
# is a model that Tripo generated, tools/blender/sources/village_tripo.glb: a diorama of a
# village of stone cottages with slate roofs, a lane, trees and a low field wall, on a
# square block of earth that rises towards the back, one material with a colour picture
# and a normal map, about 0.96 by 0.95 by 0.53 units. This script cuts the block away,
# places the village on the ridge and writes the game files, the same bytes on every run.
# The ridge and the lights are built here, as before.
# Output: assets/models/village.obj and village_lights_1.obj to village_lights_5.obj, each
# with its .mtl, and the pictures assets/textures/village.png and village_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_village.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.interpolate import poly_3d_calc

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "village"
LIGHTS_NAME = "village_lights_{night}"
NIGHT_COUNT = 5

SOURCE_FILE = "village_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space: X runs along the lane,
# Y points away from the viewer and Z is the height.
#
# The origin of these models is not their base: it is the EYE that looks at them. The game
# draws the village like the sky, centred on the camera (game::villageMatrix in
# src/game/Village.hpp), so it is only ever seen from the origin. That has two
# consequences. The numbers below are angles as much as lengths: the lane lies 257 m out
# and 27 m up, which is 6 degrees above the horizon. And the top of the ridge in front of
# the village is never seen from below: the ridge is one slope, and its upper edge hides
# the foot of the village.
#
# The numbers the game has to agree with are VILLAGE_REACH, the two of the lane and the two
# of the foot of the ridge in src/game/Village.hpp.
LANE_DISTANCE = 257.0
LANE_HEIGHT = 27.0

# The old sketch of the village had its lane this far behind the upper edge of the ridge.
# place() moves a point of that sketch out to the ridge.
SKETCH_LANE_Y = 3.2


def place(x, y, z):
    return (x, y - SKETCH_LANE_Y + LANE_DISTANCE, z + LANE_HEIGHT)


# No vertex may be further from the eye than this: the game scales the model so that this
# distance lies at nine tenths of the far plane of the camera.
REACH = 290.0

# Two corners of the source closer than this (in its own units) are one corner. The file
# has a corner of its own for every face that meets at a seam of the picture.
MERGE_DISTANCE = 0.000001

# The block of earth the village stands on: its underside, its two sides and its back.
# They are found by growing from the underside, the faces that look down and lie lower
# than BLOCK_FLOOR, across every edge to faces that look sideways or down (their normal
# has a height under BLOCK_SIDE_UP) and lie within BLOCK_HULL of the side of the bounding
# box they look out of. The growing stops at the edge where the side bends over into the
# ground on top, and the walls of a cottage near the edge are not reached: they stand
# further in. The front of the block is the same face as the field wall on it, so there
# only the part below the ground behind the wall goes (FRONT_GROUND, in units of the
# source): it lies behind the edge of the ridge anyway.
BLOCK_FLOOR = 0.005
BLOCK_SIDE_UP = 0.35
BLOCK_HULL = 0.03
FRONT_GROUND = 0.03

# Cutting the block leaves bits of it that hang on nothing. A group of faces whose bounding
# box is smaller than this across (units of the source, about 0.2 m in the game) goes too.
# The trees, the chimneys and the bits of roof are islands of their own and stay.
SPECK_SIZE = 0.005

# The source has about 40 000 triangles, for a village that is at most a few hundred
# pixels wide on the screen. The Decimate modifier (collapse, which gives the same result
# on every run) keeps this share of them.
DECIMATE_RATIO = 0.25

# The size and the place of the village. The old village of the script, as seen from the
# eye, reached from 4.7 degrees left to 5.7 degrees right of the middle (10.4 degrees
# across, which tests/VillageTests.cpp counts on for the reveal) and from the lane, 6
# degrees up, to the cap of its tower, 9 degrees up. The new one is no larger: it is
# scaled by SCALE metres per unit of the source, its middle is put in front of the eye,
# its front on the upper edge of the ridge and the ground behind its field wall at the
# height of the lane. Its cottages climb a hill towards the back, so its top reaches 9
# degrees at a smaller scale than its width would allow: it is 7.7 degrees across. The
# script stops if it is wider than MAX_WIDTH_DEGREES or higher than MAX_TOP_DEGREES.
SCALE = 36.0
FRONT_Y = 255.0
MAX_WIDTH_DEGREES = 10.4
MAX_TOP_DEGREES = 9.0

# The block is as deep as it is wide, and at that size its back would lie over 300 m out,
# past REACH. So every corner of the village is moved towards the eye along its own line of
# sight: the nearest corner stays where it is, the furthest ends at FURTHEST, and the
# distances in between shrink in proportion. Seen from the eye, which is the only place it
# is seen from, nothing changes: no angle, and no order of what hides what.
FURTHEST = 288.0

# The pictures of the source are 2048 pixels wide. The village is never closer than 257 m
# and a few hundred pixels wide at most: 1024 is plenty.
TEXTURE_SIZE = 1024

# The ridge wears a tiny piece of the picture of the village around one texel: the middle
# of the darkest and plainest square of PATCH_RADIUS texels each way in the shrunk picture
# (one every PATCH_STEP texels across and up is tried). The old ridge was near black, a
# dark shape under the sky all the way round. One metre of the ridge is this many units of u and v, so a piece of the
# rim, 280 m from its foot to its upper edge, covers 3 texels.
PATCH_RADIUS = 4
PATCH_STEP = 4
RIDGE_UV_UNITS_PER_METRE = 0.00001

# The ridge: one slope that faces the hollow. Its upper edge is level under the village and
# falls away to both sides, so the village stands on a hill and not on a bar in the sky.
RIDGE_EDGE_Y = 1.0
RIDGE_HALF_WIDTH = 100.0
RIDGE_STEP = 10.0
RIDGE_LEVEL_HALF = 30.0
RIDGE_FALL = 45.0
RIDGE_WAVE = 1.5

# The rim: the ridge does not end beside the village. It runs on as the rim of the hollow,
# the whole way round the eye, and the hill of the village rises out of it. An eye in the
# corridors never sees the rim, the fog of the hollow hides it. A camera high above the
# maze looks over that fog, and there the hill alone was a black shape with two ends and
# a lower edge, afloat in the sky. So the upper edge of the rim lies a little above the
# horizon everywhere (RIM_DEGREES, and RIM_WAVE_DEGREES up and down, RIM_WAVES times on
# the way round), and the hill is the ridge where it is higher than that.
RIM_DEGREES = 1.0
RIM_WAVE_DEGREES = 0.4
RIM_WAVES = 5
RIM_STEPS = 32
# The foot of the slope, all the way round: this far out and this far below the eye, which
# is 45 degrees down. The game draws the model 0.31 times as large, so the foot lies 62 m
# under the camera, deep in the fog for every camera up to VILLAGE_CLEAR_EYE_HEIGHT in
# src/game/Village.hpp, and no lower edge is seen. The two numbers are
# VILLAGE_FOOT_DISTANCE and VILLAGE_FOOT_DEPTH there.
RIM_FOOT_DISTANCE = 200.0
RIM_FOOT_Z = -200.0

# The windows. They are found in the picture as the eye sees it: a ray goes out from the
# eye every WINDOW_RAY_DEGREES across and up over the whole village. Where it first meets a
# wall that looks at the eye (a normal with a height under WALL_UP, pointing towards the
# eye by at least WALL_FACING) at least WALL_ABOVE_LANE over the lane (below that is the
# field wall), it has met a wall. Where the picture there is darker than WINDOW_DARK (the
# brightest channel, sRGB) it has met a window. Neighbouring rays that met a window are one
# window, and one is kept when it is WINDOW_MIN_WIDTH to WINDOW_MAX metres wide,
# WINDOW_MIN_HEIGHT to WINDOW_MAX high and fills at least WINDOW_FILL of that rectangle
# (the shadows under the eaves are long thin strips, and they are not windows), and when at
# least WINDOW_FRAMED of the rays around it met stone (a wall, not dark, and grey: its
# channels at most WALL_GREY apart). The shade under a tree is dark too, but green.
WINDOW_RAY_DEGREES = 0.01
WALL_UP = 0.3
WALL_FACING = 0.5
WALL_ABOVE_LANE = 2.0
WINDOW_DARK = 0.18
WINDOW_MIN_WIDTH = 0.3
WINDOW_MIN_HEIGHT = 0.4
WINDOW_MAX = 2.5
WINDOW_FILL = 0.4
WINDOW_FRAMED = 0.5
WALL_GREY = 0.08

# A light, as it is seen from the hollow: larger than a real window, because at 257 m a
# real one is less than two pixels wide. Its size is an angle: 0.9 by 1.1 m at the
# distance of the lane, larger in proportion further out. It stands 0.3 m in front of its
# wall: the model is drawn small and far away, where the depth buffer cannot tell two
# faces apart that lie closer together. The walls of the source are not flat, and a window
# may lie deeper than its wall, so the distance is measured along the rays from the eye to
# the corners and the middle of the light: the nearest of them meets the wall LIGHT_OFFSET
# behind the light. A light is kept only when, where it ends up, every one of those rays
# meets the village at least LIGHT_CLEAR and no more than LIGHT_REACH behind it: else
# something stands in front of it, or it hangs over the edge of its wall.
#
# The village has fewer windows the eye can see than the campaign has lights. The real
# windows come first, each at least LIGHT_SPACING rays from the lights before it (a window
# with a dark cross in it can be found twice). The rest go on the walls, one by one, each
# where it lies furthest (in rays) from every light before it, so they spread over the
# village. Such a place must have stone around it: at least WALL_AROUND of the rays in a
# square WALL_AROUND_RAYS each way, so no light goes on a chimney or a trunk.
WINDOW_WIDTH = 0.9
WINDOW_HEIGHT = 1.1
LIGHT_OFFSET = 0.3
LIGHT_CLEAR = 0.2
LIGHT_REACH = 1.5
LIGHT_SPACING = 25.0
WALL_AROUND = 0.5
WALL_AROUND_RAYS = 15

# What the game expects: how many lights each night adds.
LIGHTS_PER_NIGHT = (5, 7, 8, 11, 1)

# The lights wear the picture of the lantern, whose left half is pale, and every quad gets
# a small piece from the middle of it. The game multiplies it by amber and makes it glow.
PALE = (0.25, 0.5)
LIGHT_UV_UNITS_PER_METRE = 0.002


def ridge_top(x):
    """The height of the upper edge of the ridge at x, above the ground of the village."""
    beyond = max(abs(x) - RIDGE_LEVEL_HALF, 0.0) / (RIDGE_HALF_WIDTH - RIDGE_LEVEL_HALF)
    if beyond == 0.0:
        return 0.0
    # Smoothstep: the edge leaves the level part without a corner.
    fall = beyond * beyond * (3.0 - 2.0 * beyond)
    return -RIDGE_FALL * fall + RIDGE_WAVE * math.sin(x * 0.07) * beyond


def rim_top(x, y):
    """The height of the upper edge of the rim over the point (x, y) of the ground plan."""
    angle = RIM_DEGREES + RIM_WAVE_DEGREES * math.sin(RIM_WAVES * math.atan2(x, y))
    return math.hypot(x, y) * math.tan(math.radians(angle))


def ridge_faces():
    """The slope of the ridge, once round the eye: its corners and its faces."""
    # The upper edge, to the right as seen from the eye. Under the village it is a straight
    # line along the lane, with the height of the hill wherever the hill is higher than the
    # rim. From its two ends it goes on as a circle around the eye.
    edge = []
    steps = int(2.0 * RIDGE_HALF_WIDTH / RIDGE_STEP)
    for step in range(steps + 1):
        x, y, z = place(-RIDGE_HALF_WIDTH + step * RIDGE_STEP, RIDGE_EDGE_Y, 0.0)
        edge.append((x, y, max(z + ridge_top(x), rim_top(x, y))))
    end_x, end_y, _ = edge[-1]
    radius = math.hypot(end_x, end_y)
    end_angle = math.atan2(end_x, end_y)
    for step in range(1, RIM_STEPS):
        angle = end_angle + (2.0 * math.pi - 2.0 * end_angle) * step / RIM_STEPS
        x, y = radius * math.sin(angle), radius * math.cos(angle)
        edge.append((x, y, rim_top(x, y)))

    vertices = []
    faces = []
    for x, y, z in edge:
        nearer = RIM_FOOT_DISTANCE / math.hypot(x, y)
        vertices.append((x * nearer, y * nearer, RIM_FOOT_Z))
        vertices.append((x, y, z))
    for index in range(len(edge)):
        low = 2 * index
        following = 2 * ((index + 1) % len(edge))
        # Counter clockwise as seen from the hollow.
        faces.append((low, following, following + 1, low + 1))
    return vertices, faces


def block_faces(mesh):
    """The faces of the block of earth under the village (see BLOCK_FLOOR)."""
    corners = [corner.co for corner in mesh.verts]
    low = Vector([min(corner[axis] for corner in corners) for axis in range(3)])
    high = Vector([max(corner[axis] for corner in corners) for axis in range(3)])
    # The sides of the bounding box: the axis, the side (-1 or 1) and where it lies.
    sides = [(0, -1.0, low.x), (0, 1.0, high.x), (1, 1.0, high.y), (1, -1.0, low.y)]

    def on_side(face):
        middle = face.calc_center_median()
        if face.normal.z < -0.7 and middle.z < BLOCK_FLOOR:
            return True
        if face.normal.z > BLOCK_SIDE_UP:
            return False
        for axis, sign, plane in sides:
            if face.normal[axis] * sign > 0.7 and abs(middle[axis] - plane) < BLOCK_HULL:
                return True
        return False

    mesh.faces.index_update()
    seeds = [face for face in mesh.faces
             if face.normal.z < -0.7 and face.calc_center_median().z < BLOCK_FLOOR]
    found = {face.index for face in seeds}
    # The list itself is the queue, as in generated.islands.
    for face in seeds:
        for other in generated.neighbours(face):
            if other.index not in found and on_side(other):
                found.add(other.index)
                seeds.append(other)
    # The front of the block carries the field wall: only what lies below the ground
    # behind it goes.
    front = low.y + BLOCK_HULL
    return [face for face in seeds
            if face.calc_center_median().y > front or
            face.calc_center_median().z < FRONT_GROUND]


def clean(model):
    """Returns the village without its block as a bmesh."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    # The object of the import is moved by a few millimetres. That goes into the corners.
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)

    block = block_faces(mesh)
    bmesh.ops.delete(mesh, geom=block, context="FACES")

    specks = []
    for island in generated.islands(mesh):
        corners = [corner.co for face in island for corner in face.verts]
        across = max(max(corner[axis] for corner in corners) -
                     min(corner[axis] for corner in corners) for axis in range(3))
        if across < SPECK_SIZE:
            specks.extend(island)
    bmesh.ops.delete(mesh, geom=specks, context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")
    print(f"Village: cut {len(block)} faces of the block and {len(specks)} of specks, "
          f"{len(mesh.faces)} faces left")
    return mesh


def decimate(mesh):
    """Returns the mesh with DECIMATE_RATIO of its triangles, as a new bmesh."""
    common.reset_scene()
    data = bpy.data.meshes.new("decimate")
    mesh.to_mesh(data)
    mesh.free()
    model = bpy.data.objects.new("decimate", data)
    bpy.context.scene.collection.objects.link(model)
    modifier = model.modifiers.new("decimate", "DECIMATE")
    modifier.decimate_type = "COLLAPSE"
    modifier.ratio = DECIMATE_RATIO
    smaller = bmesh.new()
    smaller.from_object(model, bpy.context.evaluated_depsgraph_get())
    common.reset_scene()
    return smaller


def angles(point):
    """How far right of the middle and how far up a point lies, as seen from the eye, in
    degrees."""
    return (math.degrees(math.atan2(point.x, point.y)),
            math.degrees(math.atan2(point.z, math.hypot(point.x, point.y))))


def place_village(mesh):
    """Scales the village and puts it on the ridge, then moves it in along the lines of
    sight until its furthest corner lies at FURTHEST."""
    corners = [corner.co for corner in mesh.verts]
    middle_x = (min(corner.x for corner in corners) + max(corner.x for corner in corners)) / 2.0
    front = min(corner.y for corner in corners)
    for corner in mesh.verts:
        corner.co = Vector(((corner.co.x - middle_x) * SCALE,
                            FRONT_Y + (corner.co.y - front) * SCALE,
                            LANE_HEIGHT + (corner.co.z - FRONT_GROUND) * SCALE))

    seen = [angles(corner.co) for corner in mesh.verts]
    left = min(across for across, _ in seen)
    right = max(across for across, _ in seen)
    top = max(up for _, up in seen)
    print(f"Village: from {left:.2f} to {right:.2f} degrees across, up to {top:.2f} degrees")
    if right - left > MAX_WIDTH_DEGREES or top > MAX_TOP_DEGREES:
        raise RuntimeError(f"{NAME}: larger on the screen than the old village")

    distances = [corner.co.length for corner in mesh.verts]
    nearest = min(distances)
    shrink = (FURTHEST - nearest) / (max(distances) - nearest)
    for corner, distance in zip(mesh.verts, distances):
        corner.co *= (nearest + (distance - nearest) * shrink) / distance
    print(f"Village: depth along the lines of sight times {shrink:.3f}")


def ridge_patch(colour):
    """The UV middle of the darkest and plainest square of the picture (see PATCH_RADIUS)."""
    size = colour.shape[0]
    side = 2 * PATCH_RADIUS + 1
    # Every square of side by side texels, with its middle every PATCH_STEP texels: shape
    # (rows, columns, channels, side, side).
    squares = np.lib.stride_tricks.sliding_window_view(colour, (side, side), axis=(0, 1))
    squares = squares[::PATCH_STEP, ::PATCH_STEP]
    darkness = squares.max(axis=2).mean(axis=(2, 3)) + squares.std(axis=(3, 4)).sum(axis=2)
    # argmin keeps the first of equal values, row by row.
    row, column = np.unravel_index(int(np.argmin(darkness)), darkness.shape)
    row = row * PATCH_STEP + PATCH_RADIUS
    column = column * PATCH_STEP + PATCH_RADIUS
    texel = colour[row, column]
    print(f"Village: the ridge wears the texel ({texel[0]:.2f}, {texel[1]:.2f}, {texel[2]:.2f})")
    return ((column + 0.5) / size, (row + 0.5) / size)


def join_ridge(mesh, patch):
    """Adds the ridge to the village. Returns the model, one object in the scene."""
    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The village is smooth: the exporter writes the normal of every corner as the average
    # of the faces around it. Its roofs and its ground are made of thousands of small
    # faces, and one normal per face would show every one of them.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    model = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(model)

    vertices, faces = ridge_faces()
    ridge = common.create_mesh_object("ridge", vertices, faces)
    common.piece_project_uvs(ridge.data, [patch] * len(faces), RIDGE_UV_UNITS_PER_METRE)

    # The ridge goes after the village, so the faces of the village keep their order.
    for item in bpy.context.scene.objects:
        item.select_set(True)
    bpy.context.view_layer.objects.active = model
    bpy.ops.object.join()
    return model


def look_at_walls(model, colour):
    """Sends the rays of the windows out from the eye. Returns, for every ray that met a
    wall, its (column, row) and (where it met it, the normal there, whether the picture
    is dark there and whether it is grey there), and the tree of the faces, for more rays.
    """
    data = model.data
    uv_layer = data.uv_layers["uv"]
    tree = BVHTree.FromPolygons([vertex.co for vertex in data.vertices],
                                [polygon.vertices for polygon in data.polygons])
    # The ridge lies no higher than the lane, so these are corners of the village.
    seen = [angles(vertex.co) for vertex in data.vertices
            if vertex.co.z > LANE_HEIGHT + WALL_ABOVE_LANE]
    step = WINDOW_RAY_DEGREES
    first_across = math.floor(min(across for across, _ in seen) / step)
    last_across = math.ceil(max(across for across, _ in seen) / step)
    first_up = math.floor(math.degrees(math.atan2(LANE_HEIGHT, LANE_DISTANCE)) / step)
    last_up = math.ceil(max(up for _, up in seen) / step)
    height, width, _ = colour.shape

    walls = {}
    for row in range(first_up, last_up + 1):
        up = math.radians(row * step)
        for column in range(first_across, last_across + 1):
            across = math.radians(column * step)
            direction = Vector((math.sin(across) * math.cos(up),
                                math.cos(across) * math.cos(up), math.sin(up)))
            point, normal, number, _ = tree.ray_cast(Vector(), direction)
            if point is None or point.z < LANE_HEIGHT + WALL_ABOVE_LANE:
                continue
            if abs(normal.z) > WALL_UP or normal.dot(-direction) < WALL_FACING:
                continue
            polygon = data.polygons[number]
            weights = poly_3d_calc([data.vertices[index].co for index in polygon.vertices],
                                   point)
            uv = sum((uv_layer.data[loop].uv * weight
                      for loop, weight in zip(polygon.loop_indices, weights)), Vector((0, 0)))
            texel = colour[min(height - 1, max(0, int(uv.y * height))),
                           min(width - 1, max(0, int(uv.x * width)))]
            walls[(column, row)] = (point, normal, bool(texel.max() < WINDOW_DARK),
                                    bool(texel.max() - texel.min() < WALL_GREY))
    return walls, tree


def level(normal):
    """The normal of a wall without its height, of length 1."""
    flat = Vector((normal.x, normal.y, 0.0))
    flat.normalize()
    return flat


def is_stone(walls, key):
    """Whether a ray met a wall where the picture is grey and not dark."""
    return key in walls and walls[key][3] and not walls[key][2]


def stone_around(walls, keys):
    """For every ray of keys, the share of the rays in the square WALL_AROUND_RAYS each way
    around it that met stone."""
    columns = [key[0] for key in walls]
    rows = [key[1] for key in walls]
    left, bottom = min(columns) - WALL_AROUND_RAYS, min(rows) - WALL_AROUND_RAYS
    stone = np.zeros((max(rows) - bottom + WALL_AROUND_RAYS + 2,
                      max(columns) - left + WALL_AROUND_RAYS + 2))
    for key in walls:
        if is_stone(walls, key):
            stone[key[1] - bottom + 1, key[0] - left + 1] = 1.0
    # Sums over squares from a table of sums over rectangles from the corner.
    table = stone.cumsum(axis=0).cumsum(axis=1)
    side = 2 * WALL_AROUND_RAYS + 1
    shares = []
    for column, row in keys:
        top, right = row - bottom + 1 + WALL_AROUND_RAYS, column - left + 1 + WALL_AROUND_RAYS
        low, near = top - side, right - side
        shares.append((table[top, right] - table[low, right] - table[top, near] +
                       table[low, near]) / (side * side))
    return np.array(shares)


def ray_order(key):
    """Rays from the top left, row by row."""
    return (-key[1], key[0])


def find_windows(walls):
    """The windows among the rays that met a wall: for each, its middle, the normal of
    its wall and its (column, row) in rays, from the top left, row by row."""
    windows = []
    done = set()
    for start in sorted(walls, key=ray_order):
        if start in done or not walls[start][2]:
            continue
        group = [start]
        done.add(start)
        # The group grows ray by ray. The list itself is the queue.
        for column, row in group:
            for other in ((column + 1, row), (column - 1, row), (column, row + 1),
                          (column, row - 1)):
                if other in walls and walls[other][2] and other not in done:
                    done.add(other)
                    group.append(other)
        members = set(group)
        border = {other for column, row in group
                  for other in ((column + 1, row), (column - 1, row), (column, row + 1),
                                (column, row - 1)) if other not in members}
        if sum(is_stone(walls, key) for key in border) < WINDOW_FRAMED * len(border):
            continue
        middle = sum((walls[key][0] for key in group), Vector()) / len(group)
        normal = level(sum((walls[key][1] for key in group), Vector()))
        columns = max(key[0] for key in group) - min(key[0] for key in group) + 1
        rows = max(key[1] for key in group) - min(key[1] for key in group) + 1
        metres = middle.length * math.radians(WINDOW_RAY_DEGREES)
        if (WINDOW_MIN_WIDTH <= columns * metres <= WINDOW_MAX and
                WINDOW_MIN_HEIGHT <= rows * metres <= WINDOW_MAX and
                len(group) >= WINDOW_FILL * columns * rows):
            ray = (sum(key[0] for key in group) / len(group),
                   sum(key[1] for key in group) / len(group))
            windows.append((middle, normal, ray))
    return windows


def light_corners(middle, normal, offset):
    """The four corners of the light of a window, counter clockwise as seen from the eye,
    offset metres in front of the middle of the window."""
    size = middle.length / LANE_DISTANCE
    up = Vector((0.0, 0.0, 1.0))
    right = up.cross(normal)
    centre = middle + normal * offset
    half_width = right * WINDOW_WIDTH * size / 2.0
    half_height = up * WINDOW_HEIGHT * size / 2.0
    return [centre - half_width - half_height, centre + half_width - half_height,
            centre + half_width + half_height, centre - half_width + half_height]


def gaps(tree, corners):
    """How far behind each corner of a light and its middle the eye meets the village,
    along the ray. None when a ray meets nothing."""
    found = []
    for corner in corners + [sum(corners, Vector()) / 4.0]:
        point, _, _, distance = tree.ray_cast(Vector(), corner.normalized())
        if point is None:
            return None
        found.append(distance - corner.length)
    return found


def place_light(tree, middle, normal):
    """The corners of the light of a window, or None when it does not fit on its wall."""
    flat = gaps(tree, light_corners(middle, normal, 0.0))
    if flat is None:
        return None
    # The rays to the corners of the light where it ends up meet the wall in other places
    # than before, a little nearer or further: the second measure.
    corners = light_corners(middle, normal, LIGHT_OFFSET - min(flat))
    found = gaps(tree, corners)
    if found is None or min(found) < LIGHT_CLEAR or max(found) > LIGHT_REACH:
        return None
    return corners


def ray_of(corners):
    """Where the middle of a light lies as seen from the eye, in rays (column, row)."""
    across, up = angles(sum(corners, Vector()) / 4.0)
    return (across / WINDOW_RAY_DEGREES, up / WINDOW_RAY_DEGREES)


def choose_lights(walls, tree):
    """The lights of the village: (corners, (column, row) in rays), sum(LIGHTS_PER_NIGHT)
    of them, the windows first. Returns them and how many are windows."""
    lights = []
    for middle, normal, ray in find_windows(walls):
        corners = place_light(tree, middle, normal)
        if corners is None:
            continue
        ray = ray_of(corners)
        if all(math.dist(ray, other) >= LIGHT_SPACING for _, other in lights):
            lights.append((corners, ray))
    windows = len(lights)

    # The rest, on the walls. The candidates are the rays that met stone with stone around
    # them, in the order of the rays, so of two places equally far from every light the
    # first one wins.
    keys = sorted((key for key in walls if is_stone(walls, key)), key=ray_order)
    keys = [key for key, share in zip(keys, stone_around(walls, keys)) if share >= WALL_AROUND]
    rays = np.array(keys, dtype=float)
    furthest = np.full(len(keys), np.inf)
    for _, ray in lights:
        furthest = np.minimum(furthest, np.hypot(*(rays - ray).T))
    while len(lights) < sum(LIGHTS_PER_NIGHT):
        best = int(np.argmax(furthest))
        if furthest[best] < LIGHT_SPACING:
            raise RuntimeError(f"{NAME}: no room on the walls for every light: {len(lights)} "
                               f"lights, {windows} of them windows, {len(keys)} places")
        point, normal, _, _ = walls[keys[best]]
        corners = place_light(tree, point, level(normal))
        # The light is moved off its wall along the normal, and on a wall seen from the
        # side that moves it sideways too: the distances count from where it ends up.
        ray = None if corners is None else ray_of(corners)
        if ray is not None and all(math.dist(ray, other) >= LIGHT_SPACING
                                   for _, other in lights):
            lights.append((corners, ray))
            furthest = np.minimum(furthest, np.hypot(*(rays - ray).T))
        else:
            furthest[best] = -1.0
    print(f"Village: {windows} lights on windows of the picture, "
          f"{len(lights) - windows} on the walls")
    return lights, windows


def assign_nights(lights, windows):
    """Gives every light the night that lights it. The last night lights the window of
    the player: the real window of the right half that lies furthest from every other
    light, as it burns brighter than the others and its halo must be the one that is seen
    coming on. The others are lit from left to right along the village, LIGHTS_PER_NIGHT
    of them each night."""
    def alone(index):
        return min(math.dist(lights[index][1], ray)
                   for other, (_, ray) in enumerate(lights) if other != index)

    right_half = [index for index in range(windows) if lights[index][1][0] > 0.0]
    # max() keeps the first of equal values.
    own = max(right_half, key=alone)
    rest = sorted((index for index in range(len(lights)) if index != own),
                  key=lambda index: (lights[index][1][0], -lights[index][1][1]))
    nights = [(lights[own][0], NIGHT_COUNT)]
    first = 0
    for night, count in enumerate(LIGHTS_PER_NIGHT[:-1], start=1):
        nights.extend((lights[index][0], night) for index in rest[first:first + count])
        first += count
    return nights


def check_reach(name, vertices):
    furthest = max(math.sqrt(x * x + y * y + z * z) for x, y, z in vertices)
    if furthest > REACH:
        raise RuntimeError(f"{name}: a vertex lies {furthest:.1f} m out, past REACH")


def export_lights(night, lights):
    """Writes the model of the lights one night adds: one quad for each."""
    name = LIGHTS_NAME.format(night=night)
    common.reset_scene()
    vertices = []
    faces = []
    for corners, lit_on in lights:
        if lit_on != night:
            continue
        faces.append(tuple(range(len(vertices), len(vertices) + 4)))
        vertices.extend(tuple(corner) for corner in corners)

    if len(faces) != LIGHTS_PER_NIGHT[night - 1]:
        raise RuntimeError(f"{name}: {len(faces)} lights, not {LIGHTS_PER_NIGHT[night - 1]}")
    check_reach(name, vertices)

    model = common.create_mesh_object(name, vertices, faces)
    common.piece_project_uvs(model.data, [PALE] * len(faces), LIGHT_UV_UNITS_PER_METRE)
    common.assign_textured_material(model, "lantern", "lantern.png", "lantern_normal.png")
    common.export_obj(name)


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = clean(model)
    mesh = decimate(mesh)
    place_village(mesh)

    small = generated.shrink(colour, TEXTURE_SIZE)
    make_textures.save_png(small, "village.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "village_normal.png")

    model = join_ridge(mesh, ridge_patch(small))
    check_reach(NAME, [vertex.co for vertex in model.data.vertices])
    print(f"Village: {sum(len(polygon.vertices) - 2 for polygon in model.data.polygons)} "
          "triangles")
    walls, tree = look_at_walls(model, colour)
    lights, windows = choose_lights(walls, tree)

    common.assign_textured_material(model, "village", "village.png", "village_normal.png")
    common.export_obj(NAME)

    if shots:
        # From the eye, with the lens of the review camera, and from nearer. The second
        # camera stays below the lane like the eye.
        common.render_review_shots(
            NAME,
            target=(0.0, LANE_DISTANCE + 10.0, LANE_HEIGHT + 4.0),
            camera_positions=[(0.0, 0.0, 0.0), (-12.0, LANE_DISTANCE - 60.0, LANE_HEIGHT - 4.0)],
        )

    nights = assign_nights(lights, windows)
    for night in range(1, NIGHT_COUNT + 1):
        export_lights(night, nights)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
