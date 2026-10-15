# Builds the model gate_arch: the mossy stone arch that stands over the exit gate. Two
# piers of rough stones over the two pillars, a round arch of stones over the doorway,
# four iron hooks, one on each face of each pier, for the lanterns, and a slab of stone
# that closes the doorway over the door. The big lantern stands on the crown of the arch
# and the bell (build_gate_bell.py) hangs under it, in an opening of the slab.
#
# The stone is not built in this script. Its source is a model that Tripo generated,
# tools/blender/sources/gate_arch_tripo.glb: a mossy drystone arch with a small lantern on
# a hook and an iron strap on one jamb, one material with a colour picture and a normal
# map, about 0.98 m tall. The lantern and the strap come out: the game draws its own
# lanterns, and the door sinks into the ground and has no hinges. The hooks are built
# here, one on each face of each pier, so the arch has a hook wherever the game hangs a
# lantern, whichever side the player comes from.
# Output: assets/models/gate_arch.obj and gate_arch.mtl, and the pictures
# assets/textures/gate_arch.png and gate_arch_normal.png.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_gate_arch.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

import bmesh
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures

NAME = "gate_arch"

SOURCE_FILE = "gate_arch_tripo.glb"

# All sizes are in metres. The geometry is written in Blender space: X runs along the gate,
# Y is its thickness and Z is the height. After export Z becomes the game Y axis.
# The arch stands like the models gate and wall_straight: along X, centred on the origin,
# with the origin on the floor in the middle of the base. The game draws it with the
# matrix of the gate whichever side the player comes from, so everything the game hangs
# on it is on both faces: the hooks, and the lantern and the bell in the middle.
#
# The numbers the game has to agree with are in src/game/GateLamp.hpp (where the lanterns
# and the bell hang) and src/game/MazeLayout.hpp (GATE_HOUSE_HEIGHT, PILLAR_SIZE).

# Two corners of the source closer than this (in its own units) are one corner.
MERGE_DISTANCE = 0.000001

# The size. The source is too wide in its piers for a maze: at the scale that makes its
# doorway the 2 m of a cell, each pier would be 1.3 m wide and the arch 2.5 m deep, and a
# pier would stand in the corridors beside the gate, which have no collision box there.
# So the source is stretched by parts, each along X, Y and Z on its own:
#
#   - Below head height (LOW_TOP, over the 1.8 m of the player with some margin) no stone
#     is nearer to the line of the wall than the body of the player can get: the pillar
#     at each end of the gate is PILLAR_SIZE (0.3 m) wide and the body 0.6 m, so the eye
#     stays 0.45 m from the line and 0.55 m from the middle of the doorway. The stone of
#     the piers stays within PIER_DEPTH of the line, between PIER_INNER and PIER_OUTER
#     from the middle, and so 0.15 m from the eye at the closest, more than the near
#     plane of the camera (0.1 m).
#   - The doorway between the piers keeps the shape of the source: X and Z grow by about
#     the same factor (Z by TALLER more), the one that puts the innermost stone of the
#     jambs below LOW_TOP at PIER_INNER. The doorway is 1.5 m wide, a little narrower than
#     the corridor (1.7 m between the collision boxes), and the 2 m door sinks between the
#     piers.
#   - The piers beyond PIER_INNER are squeezed along X so the outermost stone below
#     LOW_TOP ends at PIER_OUTER. The pillar of the maze (0.3 m wide, 3.15 m high) stands
#     inside each pier.
#   - Y grows by the factor that puts the deepest stone below LOW_TOP at PIER_DEPTH, and
#     above HIGH_FROM by HIGH_DEPTH_SHARE of the factor of X and Z, so the arch over the
#     heads looks as deep as it is wide. In between the factor goes over evenly.
PIER_INNER = 0.75
PIER_OUTER = 1.3
PIER_DEPTH = 0.3
LOW_TOP = 2.3
HIGH_FROM = 3.0
HIGH_DEPTH_SHARE = 0.5
# Z grows by this much more than X: the crown, where the big lantern stands, is then 4.8 m
# up, high enough that the lantern shows over the walls on the hills around the gate.
TALLER = 1.07

# The piers reach this far below the floor, because the ground under a pier can be lower
# than under the middle of the gate. The lowest course of the source (up to BOTTOM_BAND
# in its own units) is stretched down to it.
FOOT_DEPTH = 0.4
BOTTOM_BAND = 0.02

# The pillar at each end of the gate (PILLAR_SIZE wide, 3.15 m tall, PILLAR_AT from the
# middle) stands on its own ground, which can be up to about 0.2 m higher than the base
# of the gate, so its head can reach PILLAR_COVER_TOP. Where the piers turn into the arch
# the source is thinner than that: from LOW_TOP up, every corner of the stone that lies
# within PILLAR_COVER of the middle of a pillar (measured along the gate and across it,
# the larger of the two) is pushed out to that distance, so the stone wraps the pillar.
# Above PILLAR_COVER_TOP the push fades out over PILLAR_COVER_FADE, so no step shows.
PILLAR_AT = 1.0
PILLAR_COVER = 0.21
PILLAR_COVER_TOP = 3.45
PILLAR_COVER_FADE = 0.25

# The rays that measure the doorway of the source go across it at these heights (in the
# units of the source, below where the arch begins to curve) and at these depths.
JAMB_HEIGHTS = [0.06 + 0.02 * step for step in range(24)]
JAMB_DEPTHS = [-0.15, -0.1, -0.05, 0.0, 0.05, 0.1, 0.15]

# The lantern and the strap of the source. A face of either has a grey texel (the
# channels at most IRON_GREY apart) and lies in its box, in the units of the source:
# (low corner, high corner).
IRON_GREY = 0.05
LANTERN_BOX = ((0.2, -0.35, 0.25), (0.42, -0.12, 0.52))
STRAP_BOX = ((-0.21, 0.0, -0.01), (-0.12, 0.15, 0.48))

# The hooks, one on each face of each pier: a square iron bar HOOK_HALF thick from the
# stone to the point the bracket lantern hangs from, and a short end that turns down
# into the ring of the lantern. The lantern hangs HOOK_ALONG from the middle of the
# gate and HOOK_OUT in front of it, with its base at HOOK_LANTERN_HEIGHT; it is 0.36 m
# tall to the top of its ring (build_gate_lantern.py). These are GATE_BRACKET_LANTERN_ALONG,
# GATE_BRACKET_LANTERN_OUT and GATE_BRACKET_LANTERN_HEIGHT in src/game/GateLamp.hpp.
# The bar leaves the stone of the jamb where a line from the end of the hook, across the
# doorway towards the pier and the wall, meets it (HOOK_TOWARDS), and reaches HOOK_GRIP
# into the stone.
HOOK_ALONG = 0.7
HOOK_OUT = 0.42
HOOK_LANTERN_HEIGHT = 2.22
LANTERN_HEIGHT = 0.36
HOOK_HALF = 0.012
HOOK_RISE = 0.03
HOOK_DROP = 0.05
HOOK_TOWARDS = Vector((1.0, -1.0, 0.0)).normalized()
HOOK_GRIP = 0.04
HOOK_REACH = 0.4

# The hooks wear the iron of the lantern of the source: a tiny piece of the picture
# around the UV middle of its darkest face, this many UV units per metre.
IRON_UV_UNITS_PER_METRE = 0.01
# The faces that close the holes the lantern and the strap leave wear the stone beside
# them in the same way, this many UV units per metre (the holes are 2 to 15 cm wide in the
# units of the source).
STONE_UV_UNITS_PER_METRE = 0.05
HOLE_MARGIN = 0.05

# The big lantern stands on the crown, drawn twice as large as the model, so its base is
# 0.4 m wide: it stands on the highest stone under that square. The bell hangs under the
# crown, in the middle of the doorway: its rope reaches BELL_GRIP into the stone over it.
BIG_LANTERN_HALF = 0.2
BELL_GRIP = 0.01

# The tympanum: a slab of stone that closes the doorway over the door, so the sky and the
# exit cell do not show over its top. The door is 2.4 m tall and sinks into the ground,
# and the slab stays: it starts under the top of the door (TYMPANUM_BOTTOM: the planks
# end unevenly, the lowest of them in the doorway at 2.31 m, so no gap shows) and is TYMPANUM_HALF thick to each side, a little more than the door, so the
# top of the closed door is inside it. Its outline follows the stone of the arch: rays
# from the middle of its bottom edge, TYMPANUM_RAYS steps over half a turn, meet the
# arch, and the outline lies TYMPANUM_GRIP further, inside the stone, so no gap shows
# where they meet. The bell hangs in the middle of the doorway, so the slab has an
# opening for it: BELL_OPENING_HALF to each side of the middle (the bell is 0.31 m wide
# and swings 0.19 m to each side at its lowest point) from BELL_OPENING_SILL up to the
# stone (the bell hangs down to 3.02 m).
TYMPANUM_BOTTOM = 2.28
TYMPANUM_HALF = 0.13
TYMPANUM_RAYS = 48
TYMPANUM_GRIP = 0.04
BELL_OPENING_HALF = 0.42
BELL_OPENING_SILL = 2.92
# The slab wears a tiny piece of the picture of the stones over the doorway, this many UV
# units per metre: the darker third of them (TYMPANUM_SHADE), so it reads as stone in the
# shadow of the arch.
TYMPANUM_UV_UNITS_PER_METRE = 0.02
TYMPANUM_SHADE = 0.3

# The source pictures are 2048 pixels wide. The arch is the landmark of the maze and is
# seen from close by, so the game gets them at 1024.
TEXTURE_SIZE = 1024


def in_box(point, box):
    low, high = box
    return all(low[axis] <= point[axis] <= high[axis] for axis in range(3))


def cut_iron(mesh, colour, uv_layer):
    """Takes the lantern and the strap out of the source. Returns the UV middle of the
    darkest face of the lantern, for the iron of the hooks."""
    iron = []
    for face in mesh.faces:
        texel = generated.texel_at(colour, face, uv_layer)
        middle = face.calc_center_median()
        if texel.max() - texel.min() < IRON_GREY and (in_box(middle, LANTERN_BOX)
                                                      or in_box(middle, STRAP_BOX)):
            iron.append(face)
    darkest = min(iron, key=lambda face: (generated.texel_at(colour, face, uv_layer).max(),
                                          face.index))
    iron_uv = (sum(loop[uv_layer].uv.x for loop in darkest.loops) / len(darkest.loops),
               sum(loop[uv_layer].uv.y for loop in darkest.loops) / len(darkest.loops))
    iron_corners = {tuple(corner.co) for face in iron for corner in face.verts}
    bmesh.ops.delete(mesh, geom=iron, context="FACES")
    # The pieces of the lantern and the specks of the source hang on nothing now. Only the
    # largest island stays: the arch.
    mesh.faces.index_update()
    arch = set(generated.largest_island(mesh))
    bmesh.ops.delete(mesh, geom=[face for face in mesh.faces if face not in arch],
                     context="FACES")
    bmesh.ops.delete(mesh, geom=[corner for corner in mesh.verts if not corner.link_faces],
                     context="VERTS")

    # Where the hook of the lantern and the strap grew out of the stone, the stone has
    # holes now. Each is closed by a fan of triangles from the middle of its edge, which
    # wear a tiny piece of the picture of a stone nearby: of the median brightness within
    # HOLE_MARGIN of that edge (project_piece), because the stone right at the hook is
    # painted dark with its shadow. The source has holes of its own under its foot, which
    # nobody sees, and they stay.
    mesh.faces.index_update()
    mesh.verts.index_update()
    for hole in generated.boundary_loops(mesh):
        if not any(tuple(corner.co) in iron_corners for corner in hole):
            continue
        middle_point = sum((corner.co for corner in hole), Vector()) / len(hole)
        reach = max((corner.co - middle_point).length for corner in hole) + HOLE_MARGIN
        stone = [face for face in mesh.faces
                 if (face.calc_center_median() - middle_point).length < reach]
        stone.sort(key=lambda face: (generated.texel_at(colour, face, uv_layer).sum(),
                                     face.index))
        typical = stone[len(stone) // 2]
        centre_uv = (sum(loop[uv_layer].uv.x for loop in typical.loops) / 3.0,
                     sum(loop[uv_layer].uv.y for loop in typical.loops) / 3.0)
        middle = mesh.verts.new(middle_point)
        # The hole is walked along the stone beside it, so the lid goes the other way.
        for index, corner in enumerate(hole):
            following = hole[(index + 1) % len(hole)]
            lid = mesh.faces.new((following, corner, middle))
            project_piece(lid, uv_layer, centre_uv, STONE_UV_UNITS_PER_METRE)
    return iron_uv


def measure_doorway(mesh):
    """Returns the middle of the doorway of the source along X, in its own units: half way
    between the jambs, each at the median of the rays that meet it."""
    tree = BVHTree.FromBMesh(mesh)
    lefts = []
    rights = []
    for height in JAMB_HEIGHTS:
        for depth in JAMB_DEPTHS:
            start = Vector((0.0, depth, height))
            left = tree.ray_cast(start, Vector((-1.0, 0.0, 0.0)), 1.0)[0]
            right = tree.ray_cast(start, Vector((1.0, 0.0, 0.0)), 1.0)[0]
            if left is not None and right is not None:
                lefts.append(left.x)
                rights.append(right.x)
    middle = (sorted(lefts)[len(lefts) // 2] + sorted(rights)[len(rights) // 2]) / 2.0
    return middle


def reshape(mesh):
    """Moves every corner from the source into the size of the game (see the numbers at
    the top). Returns the factor of X and Z in the doorway."""
    middle = measure_doorway(mesh)
    corners = list(mesh.verts)
    for corner in corners:
        corner.co.x -= middle

    # The low part is the part below LOW_TOP in the game, which depends on the factor that
    # is being found: two rounds settle it.
    factor = PIER_INNER / 0.2
    for _ in range(2):
        low = [corner for corner in corners if corner.co.z * factor * TALLER < LOW_TOP]
        inner = min(abs(corner.co.x) for corner in low)
        factor = PIER_INNER / inner
    low = [corner for corner in corners if corner.co.z * factor * TALLER < LOW_TOP]
    outer = max(abs(corner.co.x) for corner in low)
    squeeze = (PIER_OUTER - PIER_INNER) / (outer - inner)
    deep = max(abs(corner.co.y) for corner in low)
    low_depth = PIER_DEPTH / deep
    high_depth = HIGH_DEPTH_SHARE * factor
    bottom = BOTTOM_BAND * factor * TALLER

    for corner in corners:
        x, y, z = corner.co
        side = 1.0 if x >= 0.0 else -1.0
        if abs(x) <= inner:
            new_x = x * factor
        else:
            new_x = side * (PIER_INNER + (abs(x) - inner) * squeeze)
        new_z = z * factor * TALLER
        if z < BOTTOM_BAND:
            new_z = -FOOT_DEPTH + (FOOT_DEPTH + bottom) * z / BOTTOM_BAND
        share = min(1.0, max(0.0, (new_z - LOW_TOP) / (HIGH_FROM - LOW_TOP)))
        new_y = y * (low_depth + (high_depth - low_depth) * share)
        corner.co = (new_x, new_y, new_z)
    for corner in corners:
        x, y, z = corner.co
        if z < LOW_TOP or z > PILLAR_COVER_TOP + PILLAR_COVER_FADE:
            continue
        side = 1.0 if x >= 0.0 else -1.0
        along = x - side * PILLAR_AT
        reach = max(abs(along), abs(y))
        if reach >= PILLAR_COVER or reach < 0.000001:
            continue
        weight = min(1.0, (PILLAR_COVER_TOP + PILLAR_COVER_FADE - z) / PILLAR_COVER_FADE)
        push = 1.0 + (PILLAR_COVER / reach - 1.0) * weight
        corner.co = (side * PILLAR_AT + along * push, y * push, z)
    return factor


def project_piece(face, uv_layer, centre_uv, units_per_metre):
    """Gives a face a small piece of the picture around centre_uv. The face is projected
    onto its own plane, as common.face_project_uvs does, so its piece of the picture is
    never a line and never a mirror image."""
    face.normal_update()
    up = Vector((0.0, 0.0, 1.0)) - face.normal * face.normal.z
    if up.length < 0.000001:
        up = Vector((0.0, 1.0, 0.0))
    up.normalize()
    right = up.cross(face.normal)
    centre = face.calc_center_median()
    for loop in face.loops:
        offset = loop.vert.co - centre
        loop[uv_layer].uv = (centre_uv[0] + offset.dot(right) * units_per_metre,
                             centre_uv[1] + offset.dot(up) * units_per_metre)


def add_bar(mesh, start, end, uv_layer, iron_uv):
    """Adds a square iron bar from start to end, HOOK_HALF thick, with its faces looking
    outwards and the iron of the picture on them."""
    along = (end - start).normalized()
    helper = Vector((0.0, 0.0, 1.0)) if abs(along.z) < 0.9 else Vector((1.0, 0.0, 0.0))
    side = along.cross(helper).normalized() * HOOK_HALF
    up = side.cross(along).normalized() * HOOK_HALF
    ring = [side + up, -side + up, -side - up, side - up]
    near = [mesh.verts.new(start + offset) for offset in ring]
    far = [mesh.verts.new(end + offset) for offset in ring]
    middle = (start + end) / 2.0
    faces = [mesh.faces.new(near[::-1]), mesh.faces.new(far)]
    for index in range(4):
        following = (index + 1) % 4
        faces.append(mesh.faces.new((near[index], near[following], far[following],
                                     far[index])))
    for face in faces:
        face.normal_update()
        if face.normal.dot(face.calc_center_median() - middle) < 0.0:
            face.normal_flip()
        face.smooth = False
        project_piece(face, uv_layer, iron_uv, IRON_UV_UNITS_PER_METRE)


def add_hooks(mesh, uv_layer, iron_uv):
    """Adds the four hooks. Returns the points where they leave the stone."""
    tree = BVHTree.FromBMesh(mesh)
    ring_top = HOOK_LANTERN_HEIGHT + LANTERN_HEIGHT
    mounts = []
    for side_x in (-1.0, 1.0):
        for side_y in (-1.0, 1.0):
            # The end of the hook turns down into the ring of the lantern.
            tip = Vector((side_x * HOOK_ALONG, side_y * HOOK_OUT, ring_top - HOOK_DROP))
            bend = tip + Vector((0.0, 0.0, HOOK_DROP + HOOK_RISE))
            towards = Vector((side_x * HOOK_TOWARDS.x, side_y * HOOK_TOWARDS.y, 0.0))
            hit = tree.ray_cast(bend, towards, 1.0)[0]
            if hit is None:
                raise RuntimeError(f"{NAME}: no stone behind the hook at {tuple(tip)}")
            add_bar(mesh, bend, hit + towards * HOOK_GRIP, uv_layer, iron_uv)
            add_bar(mesh, tip, bend + Vector((0.0, 0.0, HOOK_HALF)), uv_layer, iron_uv)
            mounts.append(hit)
    return mounts


def crown_and_soffit(mesh):
    """Returns the height of the highest stone under the big lantern on the crown, and
    the height of the stone over the middle of the doorway."""
    top = max(corner.co.z for corner in mesh.verts
              if abs(corner.co.x) <= BIG_LANTERN_HALF and abs(corner.co.y) <= BIG_LANTERN_HALF)
    tree = BVHTree.FromBMesh(mesh)
    soffit = tree.ray_cast(Vector((0.0, 0.0, 1.0)), Vector((0.0, 0.0, 1.0)), 10.0)[0]
    return top, soffit.z


def intrados(tree, depth):
    """Returns the outline of the stone over the doorway in the plane Y = depth, as (x, z)
    from the bottom of the jamb at +X over the top to the bottom of the jamb at -X: where
    the rays of the tympanum meet the stone, TYMPANUM_GRIP further."""
    outline = []
    for step in range(TYMPANUM_RAYS + 1):
        angle = math.pi * step / TYMPANUM_RAYS
        direction = Vector((math.cos(angle), 0.0, math.sin(angle)))
        hit = tree.ray_cast(Vector((0.0, depth, TYMPANUM_BOTTOM)), direction, 3.0)[0]
        if hit is None:
            raise RuntimeError(f"{NAME}: no stone over the doorway at {math.degrees(angle)}")
        point = hit + direction * TYMPANUM_GRIP
        outline.append((point.x, point.z))
    return outline


def crossing(first, second, x):
    """The height where the line from first to second, both (x, z), passes x."""
    share = (x - first[0]) / (second[0] - first[0])
    return first[1] + (second[1] - first[1]) * share


def add_tympanum(mesh, uv_layer, colour):
    """Adds the tympanum over the door, with the opening for the bell."""
    tree = BVHTree.FromBMesh(mesh)
    soffit = tree.ray_cast(Vector((0.0, 0.0, 1.0)), Vector((0.0, 0.0, 1.0)), 10.0)[0].z
    stones = [face for face in mesh.faces
              if abs(face.calc_center_median().x) < 0.6
              and soffit - 0.2 < face.calc_center_median().z < soffit + 0.3]
    stones.sort(key=lambda face: (generated.texel_at(colour, face, uv_layer).sum(),
                                  face.index))
    shade = stones[int(len(stones) * TYMPANUM_SHADE)]
    centre_uv = (sum(loop[uv_layer].uv.x for loop in shade.loops) / 3.0,
                 sum(loop[uv_layer].uv.y for loop in shade.loops) / 3.0)

    sides = {}
    for side in (-1.0, 1.0):
        outline = intrados(tree, side * TYMPANUM_HALF)
        right = [point for point in outline if point[0] >= BELL_OPENING_HALF]
        left = [point for point in outline if point[0] <= -BELL_OPENING_HALF]
        # The outline runs from +X to -X, so the right part is its start and the left
        # part its end, and the opening is where it leaves the one and enters the other.
        top_right = crossing(right[-1], outline[len(right)], BELL_OPENING_HALF)
        top_left = crossing(outline[-len(left) - 1], left[0], -BELL_OPENING_HALF)
        points = (right + [(BELL_OPENING_HALF, top_right),
                           (BELL_OPENING_HALF, BELL_OPENING_SILL),
                           (-BELL_OPENING_HALF, BELL_OPENING_SILL),
                           (-BELL_OPENING_HALF, top_left)] + left)
        sides[side] = (points, top_right, top_left)

    faces = []

    def add(corners, outwards):
        face = mesh.faces.new([mesh.verts.new(corner) for corner in corners])
        face.normal_update()
        if face.normal.dot(outwards) < 0.0:
            face.normal_flip()
        faces.append(face)

    for side, (points, _, _) in sides.items():
        add([(x, side * TYMPANUM_HALF, z) for x, z in points], Vector((0.0, side, 0.0)))
    front, top_right_front, top_left_front = sides[-1.0]
    back, top_right_back, top_left_back = sides[1.0]
    near = -TYMPANUM_HALF
    far = TYMPANUM_HALF
    # The underside, over the door, and the three sides of the opening of the bell.
    add([(front[0][0], near, TYMPANUM_BOTTOM), (back[0][0], far, TYMPANUM_BOTTOM),
         (back[-1][0], far, TYMPANUM_BOTTOM), (front[-1][0], near, TYMPANUM_BOTTOM)],
        Vector((0.0, 0.0, -1.0)))
    for x, top_front, top_back in ((BELL_OPENING_HALF, top_right_front, top_right_back),
                                   (-BELL_OPENING_HALF, top_left_front, top_left_back)):
        add([(x, near, BELL_OPENING_SILL), (x, far, BELL_OPENING_SILL), (x, far, top_back),
             (x, near, top_front)], Vector((-x, 0.0, 0.0)))
    sill = BELL_OPENING_SILL
    add([(BELL_OPENING_HALF, near, sill), (BELL_OPENING_HALF, far, sill),
         (-BELL_OPENING_HALF, far, sill), (-BELL_OPENING_HALF, near, sill)],
        Vector((0.0, 0.0, 1.0)))

    faces = bmesh.ops.triangulate(mesh, faces=faces, quad_method="BEAUTY",
                                  ngon_method="BEAUTY")["faces"]
    for face in faces:
        face.smooth = False
        project_piece(face, uv_layer, centre_uv, TYMPANUM_UV_UNITS_PER_METRE)


def check(mesh, mounts):
    """Stops when a stone below head height stands where the player walks."""
    for corner in mesh.verts:
        x, y, z = corner.co
        if z < LOW_TOP and not (PIER_INNER - 0.0001 <= abs(x) <= PIER_OUTER + 0.0001
                                and abs(y) <= PIER_DEPTH + 0.0001):
            raise RuntimeError(f"{NAME}: a stone at {tuple(corner.co)} is in the way")
    # A hook reaches the stone within HOOK_REACH of the point its lantern hangs from.
    for mount in mounts:
        reach = (Vector((abs(mount.x), abs(mount.y))) - Vector((HOOK_ALONG, HOOK_OUT))).length
        if abs(mount.x) < HOOK_ALONG or reach > HOOK_REACH:
            raise RuntimeError(f"{NAME}: a hook leaves the stone at {tuple(mount)}")
    zs = [corner.co.z for corner in mesh.verts]
    if abs(min(zs) + FOOT_DEPTH) > 0.0001:
        raise RuntimeError(f"{NAME}: the foot is at {min(zs):.4f}, not {-FOOT_DEPTH}")


def build(shots):
    model, colour, normal = generated.import_source(SOURCE_FILE)
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    uv_layer = mesh.loops.layers.uv.active

    iron_uv = cut_iron(mesh, colour, uv_layer)
    factor = reshape(mesh)
    # The stone is smooth: the exporter writes the normal of every corner as the average
    # of the faces around it. The hooks are flat.
    for face in mesh.faces:
        face.smooth = True
    crown, soffit = crown_and_soffit(mesh)
    add_tympanum(mesh, uv_layer, colour)
    mounts = add_hooks(mesh, uv_layer, iron_uv)
    bmesh.ops.triangulate(mesh, faces=mesh.faces[:])
    check(mesh, mounts)

    zs = [corner.co.z for corner in mesh.verts]
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    print(f"{NAME}: doorway factor {factor:.3f}, {min(xs):.3f} to {max(xs):.3f} along, "
          f"{min(ys):.3f} to {max(ys):.3f} deep, {max(zs):.3f} m high, "
          f"{len(mesh.faces)} triangles")
    # The two heights the game hangs the big lantern and the bell by
    # (GATE_COTE_LANTERN_HEIGHT and GATE_BELL_HEIGHT in src/game/GateLamp.hpp).
    print(f"{NAME}: the crown under the big lantern at {crown:.3f} m, the stone over the "
          f"doorway at {soffit:.3f} m, the bell pivot at {soffit + BELL_GRIP:.3f} m")
    for mount in mounts:
        print(f"{NAME}: a hook leaves the stone at ({mount.x:.3f}, {mount.y:.3f}, "
              f"{mount.z:.3f})")

    make_textures.save_png(generated.shrink(colour, TEXTURE_SIZE), "gate_arch.png")
    generated.save_normal_map(normal, TEXTURE_SIZE, "gate_arch_normal.png")

    common.reset_scene()
    data = bpy.data.meshes.new(NAME)
    mesh.to_mesh(data)
    mesh.free()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    arch = bpy.data.objects.new(NAME, data)
    bpy.context.scene.collection.objects.link(arch)
    common.assign_textured_material(arch, "gate_arch", "gate_arch.png", "gate_arch_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views: the whole arch from the front and a little to the side, and the
        # doorway from below, as the player sees it.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 2.4),
            camera_positions=[(3.5, -9.0, 2.6), (0.5, -3.0, 1.7)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
