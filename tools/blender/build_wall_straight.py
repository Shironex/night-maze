# Builds the three models of one wall segment of the maze, 2 m long and 3 m high:
#   wall_straight         the wall as it was built,
#   wall_straight_crown   the same wall with twigs of stone growing out of its coping,
#   wall_straight_broken  the wall with one coping stone lost and the next one askew.
# Output: assets/models/<name>.obj and <name>.mtl for each of the three.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_wall_straight.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "wall_straight"
CROWN_NAME = "wall_straight_crown"
BROKEN_NAME = "wall_straight_broken"

# All sizes are in metres. The geometry is written in Blender space: X is the length of the
# wall, Y is its thickness and Z is the height. After export Z becomes the game Y axis.

# The segment fills one side of a 2 m maze cell, centred on the origin.
HALF_LENGTH = 1.0
# Total height. The origin is on the floor, in the middle of the base.
HEIGHT = 3.0

# The body is the thin middle part. The plinth at the bottom and the coping at the top are
# thicker, so they stand out by 0.04 m on both sides.
BODY_HALF_THICKNESS = 0.1
TRIM_HALF_THICKNESS = 0.14
PLINTH_HEIGHT = 0.25
COPING_HEIGHT = 0.15

# ---- the broken look ----
# The coping is two pieces with a gap of 0.9 m between them. One stone of 0.26 m is gone
# from the gap, and the stone of 0.6 m beside it lies askew in the rest of it.
GAP_START = -0.46
GAP_END = 0.44
ASKEW_START = -0.2
ASKEW_END = 0.4
# The askew stone is a little smaller than the coping it broke out of.
ASKEW_HALF_THICKNESS = 0.135
ASKEW_BOTTOM = 2.86
# It is turned about its own middle: first about the Y axis (one end dips into the body,
# the other lifts off it), then about the X axis (it leans over one face of the wall).
ASKEW_CENTRE = (0.1, 0.0, 2.92)
ASKEW_TURN_Y = math.radians(5.0)
ASKEW_TURN_X = math.radians(-4.0)

# ---- the crowned look ----
# The twigs are placed by a random number generator with this seed, so every run of the
# script writes the same twigs.
TWIG_SEED = 7
# A twig is a prism with this many sides that gets thinner towards its tip.
TWIG_SIDES = 4
# The twigs stand between these two places along the wall: 0.28 m from each end, so the
# caps of the pillars stay clean.
TWIG_FIRST_X = -0.72
TWIG_LAST_X = 0.72
# They start 1 cm below the top of the coping, so no gap shows under a leaning one.
TWIG_BASE_Z = HEIGHT - 0.01
# Height of a twig, measured straight up.
TWIG_MIN_HEIGHT = 0.25
TWIG_MAX_HEIGHT = 0.6
# Half the width of a twig at its base. The tip is a few millimetres wide.
TWIG_MIN_RADIUS = 0.03
TWIG_MAX_RADIUS = 0.046
# A fork starts at 45 to 70 % of the height of its twig.
FORK_MIN_START = 0.45
FORK_MAX_START = 0.7
# From one twig to the next along the wall.
TWIG_MIN_STEP = 0.09
TWIG_MAX_STEP = 0.22

# The twigs lean, so their faces are slanted and box_project_uvs does not fit them. The
# two new looks use face_project_uvs with the density of the walls. On a face that is
# perpendicular to an axis it gives what box_project_uvs gives (only the picture on an
# underside is turned by half a turn), so the stone of the body looks as on the plain wall.


def add_wall(vertices, faces, broken):
    """Appends the plinth, the body and the coping of the wall."""
    # Heights at which one part ends and the next one starts.
    plinth_top = PLINTH_HEIGHT
    coping_bottom = HEIGHT - COPING_HEIGHT

    # All boxes end at x = -1 and x = +1, so both ends of the wall are flat and the
    # segment can butt against a pillar.

    # Plinth. Its underside lies on the floor and is never seen.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -TRIM_HALF_THICKNESS, 0.0),
        (HALF_LENGTH, TRIM_HALF_THICKNESS, plinth_top),
        skip=("-z",),
    )
    # Body. Its underside is covered by the plinth. Its top is covered by the coping,
    # except on the broken wall, where it is what the gap shows.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -BODY_HALF_THICKNESS, plinth_top),
        (HALF_LENGTH, BODY_HALF_THICKNESS, coping_bottom),
        skip=("-z",) if broken else ("-z", "+z"),
    )
    if not broken:
        # Coping. All six sides can be seen: the underside shows where it overhangs the
        # body.
        common.add_box(
            vertices,
            faces,
            (-HALF_LENGTH, -TRIM_HALF_THICKNESS, coping_bottom),
            (HALF_LENGTH, TRIM_HALF_THICKNESS, HEIGHT),
        )
        return

    # The two pieces of coping that are left, each with all six sides: the ends that
    # face the gap can be seen.
    common.add_box(
        vertices,
        faces,
        (-HALF_LENGTH, -TRIM_HALF_THICKNESS, coping_bottom),
        (GAP_START, TRIM_HALF_THICKNESS, HEIGHT),
    )
    common.add_box(
        vertices,
        faces,
        (GAP_END, -TRIM_HALF_THICKNESS, coping_bottom),
        (HALF_LENGTH, TRIM_HALF_THICKNESS, HEIGHT),
    )
    # The askew stone: a box like the others, then every corner of it is turned.
    first = len(vertices)
    common.add_box(
        vertices,
        faces,
        (ASKEW_START, -ASKEW_HALF_THICKNESS, ASKEW_BOTTOM),
        (ASKEW_END, ASKEW_HALF_THICKNESS, HEIGHT),
    )
    for index in range(first, len(vertices)):
        vertices[index] = turn_askew(vertices[index])


def turn_askew(point):
    """Turns a corner of the askew stone about ASKEW_CENTRE, about Y and then about X."""
    x = point[0] - ASKEW_CENTRE[0]
    y = point[1] - ASKEW_CENTRE[1]
    z = point[2] - ASKEW_CENTRE[2]
    cos_y, sin_y = math.cos(ASKEW_TURN_Y), math.sin(ASKEW_TURN_Y)
    x, z = x * cos_y + z * sin_y, -x * sin_y + z * cos_y
    cos_x, sin_x = math.cos(ASKEW_TURN_X), math.sin(ASKEW_TURN_X)
    y, z = y * cos_x - z * sin_x, y * sin_x + z * cos_x
    return (x + ASKEW_CENTRE[0], y + ASKEW_CENTRE[1], z + ASKEW_CENTRE[2])


def random_numbers(seed):
    """Returns a function that gives the next random number from 0 up to, not including, 1.

    A small generator of 32 bit whole numbers (mulberry32), written out here so that the
    twigs do not depend on the random module of a Python version. Python numbers do not
    wrap around at 32 bits by themselves, which is what the masks are for.
    """
    mask = 0xFFFFFFFF
    state = seed & mask

    def following():
        nonlocal state
        state = (state + 0x6D2B79F5) & mask
        mixed = ((state ^ (state >> 15)) * (1 | state)) & mask
        mixed = ((mixed + (((mixed ^ (mixed >> 7)) * (61 | mixed)) & mask)) & mask) ^ mixed
        return ((mixed ^ (mixed >> 14)) & mask) / 4294967296

    return following


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def scaled(a, factor):
    return (a[0] * factor, a[1] * factor, a[2] * factor)


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def unit(a):
    return scaled(a, 1.0 / math.sqrt(dot(a, a)))


def between(a, b, share):
    """The point that lies `share` of the way from a to b."""
    return add(a, scaled(sub(b, a), share))


def add_face_away_from(vertices, faces, corners, inside):
    """Appends a face whose normal points away from the point `inside`."""
    a, b, c = (vertices[index] for index in corners[:3])
    normal = cross(sub(b, a), sub(c, a))
    if dot(normal, sub(a, inside)) < 0.0:
        corners = tuple(reversed(corners))
    faces.append(tuple(corners))


def add_twig(vertices, faces, base, tip, base_radius, tip_radius, turn):
    """Appends one twig: a closed prism from `base` to `tip` that gets thinner.

    The two ends are rings of TWIG_SIDES corners around the line from base to tip, turned
    about it by `turn` (an angle). Both ends are closed. The base of a twig is hidden in
    the coping or in the twig it forks from, but a leaning ring can stick out a little,
    and through an open end one would look into the prism.
    """
    along = unit(sub(tip, base))
    # Two directions across the twig. The helper direction must not point along it.
    helper = (1.0, 0.0, 0.0) if abs(along[0]) < 0.9 else (0.0, 1.0, 0.0)
    side = unit(sub(helper, scaled(along, dot(helper, along))))
    other = cross(along, side)

    rings = []
    for centre, radius in ((base, base_radius), (tip, tip_radius)):
        ring = []
        for corner in range(TWIG_SIDES):
            angle = corner * 2.0 * math.pi / TWIG_SIDES + turn
            ring.append(len(vertices))
            vertices.append(
                add(
                    centre,
                    add(
                        scaled(other, math.cos(angle) * radius),
                        scaled(side, math.sin(angle) * radius),
                    ),
                )
            )
        rings.append(ring)

    middle = between(base, tip, 0.5)
    for corner in range(TWIG_SIDES):
        following = (corner + 1) % TWIG_SIDES
        # One side: flat, because both rings are turned by the same angle.
        add_face_away_from(
            vertices,
            faces,
            (rings[0][corner], rings[0][following], rings[1][following], rings[1][corner]),
            middle,
        )
    add_face_away_from(vertices, faces, rings[0], middle)
    add_face_away_from(vertices, faces, rings[1], middle)


def add_twigs(vertices, faces):
    """Appends the twigs of the crowned wall: a row along the coping, each with forks."""
    rnd = random_numbers(TWIG_SEED)
    full_turn = 2.0 * math.pi

    x = TWIG_FIRST_X
    while x < TWIG_LAST_X:
        # The order of the rnd() calls is part of the model: changing it moves every
        # twig after the change.
        height = TWIG_MIN_HEIGHT + rnd() * (TWIG_MAX_HEIGHT - TWIG_MIN_HEIGHT)
        base = (x, (rnd() - 0.5) * 0.16, TWIG_BASE_Z)
        # The twig leans: its tip is off to the side by a share of its height.
        lean_x = (rnd() - 0.5) * 0.7 * height
        lean_y = (rnd() - 0.5) * 0.55 * height
        tip = add(base, (lean_x, lean_y, height))
        radius = TWIG_MIN_RADIUS + rnd() * (TWIG_MAX_RADIUS - TWIG_MIN_RADIUS)
        add_twig(vertices, faces, base, tip, radius, 0.004, rnd() * full_turn)

        # One or two forks: thinner twigs that leave the first one sideways and upwards.
        forks = 1 + int(rnd() * 2.0)
        for _ in range(forks):
            start = FORK_MIN_START + rnd() * (FORK_MAX_START - FORK_MIN_START)
            fork_base = between(base, tip, start)
            direction_x = (rnd() - 0.5) * 1.8
            direction_y = (rnd() - 0.5) * 1.3
            direction = unit((direction_x, direction_y, 0.5 + rnd() * 0.7))
            fork_tip = add(fork_base, scaled(direction, height * (0.3 + rnd() * 0.35)))
            add_twig(vertices, faces, fork_base, fork_tip, radius * 0.55, 0.003, rnd() * full_turn)

            # Four forks out of ten carry a last short twig of their own.
            if rnd() < 0.4:
                spur_base = between(fork_base, fork_tip, 0.6)
                spur_x = (rnd() - 0.5) * 2.0
                spur_y = rnd() - 0.5
                spur_tip = add(spur_base, scaled(unit((spur_x, spur_y, 0.8)), height * 0.18))
                add_twig(
                    vertices, faces, spur_base, spur_tip, radius * 0.3, 0.002, rnd() * full_turn
                )

        x += TWIG_MIN_STEP + rnd() * (TWIG_MAX_STEP - TWIG_MIN_STEP)


def finish(name, vertices, faces, slanted, shots):
    """Turns the lists into a textured model, exports it and renders the review shots."""
    model = common.create_mesh_object(name, vertices, faces)
    if slanted:
        common.face_project_uvs(model.data, common.METRES_PER_UV_UNIT)
    else:
        common.box_project_uvs(model.data)
    common.assign_textured_material(model, "wall_stone", "wall_stone.png", "wall_stone_normal.png")
    common.export_obj(name)
    print(name, "has", sum(len(face) - 2 for face in faces), "triangles")

    if shots:
        # Two views aimed at the top of the wall, where the three looks differ: from the
        # front and a little to the side, and from the floor near one end, looking up.
        common.render_review_shots(
            name,
            target=(0.0, 0.0, 2.4),
            camera_positions=[(3.0, -6.5, 3.4), (-2.2, -2.4, 1.7)],
        )


def build(shots):
    common.reset_scene()
    vertices = []
    faces = []
    add_wall(vertices, faces, broken=False)
    finish(NAME, vertices, faces, slanted=False, shots=shots)

    common.reset_scene()
    vertices = []
    faces = []
    add_wall(vertices, faces, broken=False)
    add_twigs(vertices, faces)
    finish(CROWN_NAME, vertices, faces, slanted=True, shots=shots)

    common.reset_scene()
    vertices = []
    faces = []
    add_wall(vertices, faces, broken=True)
    finish(BROKEN_NAME, vertices, faces, slanted=True, shots=shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
