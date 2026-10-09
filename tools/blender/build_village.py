# Builds the village on the ridge: fourteen houses, the chapel with its tower, the well and
# the lamp posts of the lane, on the far rim of the hollow. And five small models of light:
# the windows and the lamp heads that are lit after each night of the campaign.
# Output: assets/models/village.obj and village_lights_1.obj to village_lights_5.obj, each
# with its .mtl.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_village.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "village"
LIGHTS_NAME = "village_lights_{night}"
NIGHT_COUNT = 5

# All sizes are in metres. The geometry is written in Blender space: X runs along the lane,
# Y points away from the viewer and Z is the height.
#
# The origin of these models is not their base: it is the EYE that looks at them. The game
# draws the village like the sky, centred on the camera (game::villageMatrix in
# src/game/Village.hpp), so it is only ever seen from the origin. That has three
# consequences. The numbers below are angles as much as lengths: the lane lies 257 m out
# and 27 m up, which is 6 degrees above the horizon, and the cap of the tower ends near 9.
# No face that looks away from the origin is built. And the top of the ridge, with the lane
# on it, is never seen from below: the ridge is one slope, and the lamp posts mark the lane.
#
# The numbers the game has to agree with are VILLAGE_REACH and the two angles in
# src/game/Village.hpp.
LANE_DISTANCE = 257.0
LANE_HEIGHT = 27.0

# The houses, the chapel and the lamps are written as on the sketch: x along the lane, y
# from the near edge of the ridge towards the back, heights above the ground of the ridge.
# place() moves such a point out to the ridge.
SKETCH_LANE_Y = 3.2


def place(x, y, z):
    return (x, y - SKETCH_LANE_Y + LANE_DISTANCE, z + LANE_HEIGHT)


# No vertex may be further from the eye than this: the game scales the model so that this
# distance lies at nine tenths of the far plane of the camera.
REACH = 290.0

# A house: the middle of its floor (x, y), width, depth, the height of its walls, the
# height of its roof, the direction of its ridge ("x": along the lane, the front shows
# a slope and a chimney. "y": the front shows a gable) and its windows. A window is
# (x from the middle of the house, the height of its sill, the night that lights it).
FRONT_HOUSES = (
    (-18.0, 6.0, 4.4, 5.0, 3.4, 2.2, "x", ((-1.0, 1.5, 1), (1.0, 1.5, 1))),
    (-12.8, 7.2, 3.6, 4.4, 3.0, 2.5, "y", ((0.0, 1.4, 1),)),
    (-8.2, 5.6, 4.8, 4.8, 3.6, 2.0, "x", ((-1.3, 1.5, 2), (1.3, 1.5, 2), (0.0, 2.8, 2))),
    (-3.4, 7.6, 3.8, 4.4, 3.2, 2.4, "y", ((0.0, 1.5, 2),)),
    (1.4, 6.0, 4.2, 4.6, 3.4, 2.1, "x", ((-1.0, 1.5, 3), (1.0, 1.5, 3))),
    (14.2, 6.4, 4.2, 4.6, 3.3, 2.2, "x", ((-1.0, 1.5, 4), (1.0, 1.5, 4))),
    (18.8, 8.2, 3.6, 4.2, 3.0, 2.4, "y", ((0.0, 1.4, 4),)),
    # The last house of the lane. Its upper window is the one of the player: night 5.
    (23.0, 6.0, 4.6, 5.0, 4.4, 2.2, "x", ((-1.1, 1.5, 4), (1.1, 1.5, 4), (0.0, 3.1, 5))),
)
BACK_HOUSES = (
    (-15.5, 12.5, 4.0, 4.2, 3.4, 2.1, "x", ((0.0, 2.2, 2),)),
    (-6.0, 12.8, 4.4, 4.4, 3.6, 2.2, "x", ((-1.0, 2.4, 3),)),
    (0.0, 13.2, 3.6, 4.0, 3.4, 2.3, "y", ((0.0, 2.3, 3),)),
    (4.6, 12.6, 4.0, 4.4, 3.6, 2.1, "x", ((0.0, 2.4, 4),)),
    (12.0, 12.8, 4.2, 4.2, 3.4, 2.2, "x", ((1.0, 2.3, 4),)),
    (18.5, 13.0, 3.8, 4.0, 3.2, 2.2, "y", ((0.0, 2.2, 4),)),
)
CHAPEL = (8.0, 7.6, 4.6, 7.0, 5.6, 3.0, "y", ((0.0, 2.6, 3),))

# The tower of the chapel, in front of its nave: the middle of its floor, the side of its
# square, the height of its walls and of its cap, and the night that lights the belfry.
TOWER_X = 8.0
TOWER_Y = 3.2
TOWER_HALF = 1.3
TOWER_HEIGHT = 10.5
TOWER_CAP = 3.2
TOWER_NIGHT = 3
BELFRY_WIDTH = 1.0
BELFRY_HEIGHT = 1.5
BELFRY_BELOW_TOP = 1.9

# Every building reaches this far into the ground, so its foot is behind the edge of the
# ridge, and every roof stands out this far over its walls.
FOOT = 0.3
EAVES = 0.2
EAVES_SLOPE = 0.3

# A window as it is seen from the hollow: larger than a real one, because at 257 m a real
# one is less than two pixels wide. It stands 0.3 m in front of its wall: the model is
# drawn small and far away, where the depth buffer cannot tell two faces apart that lie
# closer together.
WINDOW_WIDTH = 0.9
WINDOW_HEIGHT = 1.1
LIGHT_OFFSET = 0.3

# The well, on the lane: a round wall of eight sides, two posts and a small roof.
WELL_X = -5.6
WELL_Y = 3.0
WELL_RADIUS = 0.75
WELL_SIDES = 8
WELL_WALL = 0.8
WELL_POST_X = 0.74
WELL_POST_HALF = 0.07
WELL_POST_TOP = 1.9
WELL_ROOF_HALF_LENGTH = 1.05
WELL_ROOF_HALF_THICKNESS = 0.5
WELL_ROOF_TOP = 2.5

# The lamps of the lane: (x, the night that lights it). They stand on the near side of the
# lane, in front of every house. The head of a lamp is one quad of light, like a window.
LAMPS = ((-21.0, 1), (-16.2, 1), (-11.0, 2), (-7.4, 2), (-1.6, 3), (3.6, 3), (11.6, 4),
         (16.4, 4), (21.0, 4))
LAMP_Y = 2.2
LAMP_POST_HALF = 0.12
LAMP_POST_TOP = 2.5
LAMP_HEAD_WIDTH = 0.5
LAMP_HEAD_HEIGHT = 0.6

# The ridge: one slope that faces the hollow. Its upper edge is level under the village and
# falls away to both sides, below the horizon, so the village stands on a hill and not on
# a bar in the sky. Its lower edge lies below the horizon everywhere.
RIDGE_EDGE_Y = 1.0
RIDGE_HALF_WIDTH = 100.0
RIDGE_STEP = 10.0
RIDGE_LEVEL_HALF = 30.0
RIDGE_FALL = 45.0
RIDGE_WAVE = 1.5
RIDGE_FOOT_Z = -45.0
RIDGE_FOOT_NEARER = 100.0

# What the game expects: how many lights each night adds.
LIGHTS_PER_NIGHT = (5, 7, 8, 11, 1)

# Every model here wears the picture of the lantern, whose left half is pale and whose
# right half is dark, and every face gets a small piece from the middle of one half. At
# 257 m no stone could be told from another anyway. What the two halves give is two
# tones: the walls are PALE, the roofs, the posts and the ridge are DARK, and the game
# multiplies both by one dim colour (VILLAGE_TINT in src/game/VillageRenderer.cpp). The
# lights are pale too, and the game makes them glow.
PALE = (0.25, 0.5)
DARK = (0.75, 0.5)
# One metre of the model is this many units of u and v. The largest face, a piece of the
# ridge, is under 80 m long, so it stays inside its half of the picture.
UV_UNITS_PER_METRE = 0.002


def ridge_top(x):
    """The height of the upper edge of the ridge at x, above the ground of the village."""
    beyond = max(abs(x) - RIDGE_LEVEL_HALF, 0.0) / (RIDGE_HALF_WIDTH - RIDGE_LEVEL_HALF)
    if beyond == 0.0:
        return 0.0
    # Smoothstep: the edge leaves the level part without a corner.
    fall = beyond * beyond * (3.0 - 2.0 * beyond)
    return -RIDGE_FALL * fall + RIDGE_WAVE * math.sin(x * 0.07) * beyond


def add_part(vertices, faces, tones, tone, add, *arguments, **options):
    """Calls one of the helpers that append faces (common.add_box, common.add_roof,
    common.add_pyramid) and lists `tone` for every face it appended."""
    before = len(faces)
    add(vertices, faces, *arguments, **options)
    tones.extend([tone] * (len(faces) - before))


def add_ridge(vertices, faces, tones):
    steps = int(2.0 * RIDGE_HALF_WIDTH / RIDGE_STEP)
    first = len(vertices)
    for step in range(steps + 1):
        x = -RIDGE_HALF_WIDTH + step * RIDGE_STEP
        foot = place(x, RIDGE_EDGE_Y - RIDGE_FOOT_NEARER, 0.0)
        vertices.append((foot[0], foot[1], RIDGE_FOOT_Z))
        vertices.append(place(x, RIDGE_EDGE_Y, ridge_top(x)))
    for step in range(steps):
        low = first + 2 * step
        # Counter clockwise as seen from the hollow.
        faces.append((low, low + 2, low + 3, low + 1))
        tones.append(DARK)


def add_house(vertices, faces, tones, lights, house):
    """Appends one house and lists its windows in `lights`."""
    x, y, width, depth, height, roof, ridge, windows = house
    x0, x1 = x - width / 2.0, x + width / 2.0
    y0, y1 = y - depth / 2.0, y + depth / 2.0

    # The walls. The floor, the back and the top (under the roof) are never seen.
    add_part(
        vertices, faces, tones, PALE, common.add_box,
        place(x0, y0, -FOOT), place(x1, y1, height), skip=("-z", "+z", "+y"),
    )
    centre = place(x, y, 0.0)
    if ridge == "y":
        add_part(
            vertices, faces, tones, DARK, common.add_roof,
            width / 2.0 + EAVES, depth / 2.0 + EAVES,
            LANE_HEIGHT + height, LANE_HEIGHT + height + roof, False, centre=centre[:2],
        )
    else:
        add_part(
            vertices, faces, tones, DARK, common.add_roof,
            width / 2.0 + EAVES, depth / 2.0 + EAVES_SLOPE,
            LANE_HEIGHT + height, LANE_HEIGHT + height + roof, False, centre=centre[:2],
            ridge_along_x=True,
        )
        # The chimney, behind the ridge near one end of it.
        add_part(
            vertices, faces, tones, PALE, common.add_box,
            place(x1 - 0.9, y + 0.4, height + roof * 0.4),
            place(x1 - 0.45, y + 0.85, height + roof + 0.6),
            skip=("-z", "+y"),
        )
    for along, sill, night in windows:
        lights.append((x + along, y0, sill, WINDOW_WIDTH, WINDOW_HEIGHT, night))


def add_tower(vertices, faces, tones, lights):
    add_part(
        vertices, faces, tones, PALE, common.add_box,
        place(TOWER_X - TOWER_HALF, TOWER_Y - TOWER_HALF, -FOOT),
        place(TOWER_X + TOWER_HALF, TOWER_Y + TOWER_HALF, TOWER_HEIGHT),
        skip=("-z", "+z", "+y"),
    )
    centre = place(TOWER_X, TOWER_Y, 0.0)
    add_part(
        vertices, faces, tones, DARK, common.add_pyramid,
        centre[0], TOWER_HALF + EAVES, LANE_HEIGHT + TOWER_HEIGHT,
        LANE_HEIGHT + TOWER_HEIGHT + TOWER_CAP, centre_y=centre[1],
    )
    lights.append((TOWER_X, TOWER_Y - TOWER_HALF, TOWER_HEIGHT - BELFRY_BELOW_TOP,
                   BELFRY_WIDTH, BELFRY_HEIGHT, TOWER_NIGHT))


def add_well(vertices, faces, tones):
    # The round wall: its sides, and its top as one face. Seen from below the top is
    # hidden, but the well is small enough to keep whole.
    first = len(vertices)
    for z in (-FOOT, WELL_WALL):
        for side in range(WELL_SIDES):
            angle = 2.0 * math.pi * side / WELL_SIDES
            vertices.append(place(WELL_X + WELL_RADIUS * math.cos(angle),
                                  WELL_Y + WELL_RADIUS * math.sin(angle), z))
    for side in range(WELL_SIDES):
        following = (side + 1) % WELL_SIDES
        faces.append((first + side, first + following, first + WELL_SIDES + following,
                      first + WELL_SIDES + side))
    faces.append(tuple(first + WELL_SIDES + side for side in range(WELL_SIDES)))
    tones.extend([PALE] * (WELL_SIDES + 1))

    for side in (-1.0, 1.0):
        post_x = WELL_X + side * WELL_POST_X
        add_part(
            vertices, faces, tones, DARK, common.add_box,
            place(post_x - WELL_POST_HALF, WELL_Y - WELL_POST_HALF, 0.0),
            place(post_x + WELL_POST_HALF, WELL_Y + WELL_POST_HALF, WELL_POST_TOP),
            skip=("-z", "+z", "+y"),
        )
    centre = place(WELL_X, WELL_Y, 0.0)
    add_part(
        vertices, faces, tones, DARK, common.add_roof,
        WELL_ROOF_HALF_LENGTH, WELL_ROOF_HALF_THICKNESS,
        LANE_HEIGHT + WELL_POST_TOP, LANE_HEIGHT + WELL_ROOF_TOP, True, centre=centre[:2],
    )


def add_lamps(vertices, faces, tones, lights):
    for x, night in LAMPS:
        add_part(
            vertices, faces, tones, DARK, common.add_box,
            place(x - LAMP_POST_HALF, LAMP_Y - LAMP_POST_HALF, -FOOT),
            place(x + LAMP_POST_HALF, LAMP_Y + LAMP_POST_HALF, LAMP_POST_TOP),
            skip=("-z", "+z", "+y"),
        )
        lights.append((x, LAMP_Y - LAMP_POST_HALF, LAMP_POST_TOP - LAMP_HEAD_HEIGHT / 2.0,
                       LAMP_HEAD_WIDTH, LAMP_HEAD_HEIGHT, night))


def check_reach(name, vertices):
    furthest = max(math.sqrt(x * x + y * y + z * z) for x, y, z in vertices)
    if furthest > REACH:
        raise RuntimeError(f"{name}: a vertex lies {furthest:.1f} m out, past REACH")


def export_lights(night, lights):
    """Writes the model of the lights one night adds: one quad for each, facing the eye."""
    name = LIGHTS_NAME.format(night=night)
    common.reset_scene()
    vertices = []
    faces = []
    for x, wall_y, bottom, width, height, lit_on in lights:
        if lit_on != night:
            continue
        first = len(vertices)
        y = wall_y - LIGHT_OFFSET
        vertices.extend(
            [
                place(x - width / 2.0, y, bottom),
                place(x + width / 2.0, y, bottom),
                place(x + width / 2.0, y, bottom + height),
                place(x - width / 2.0, y, bottom + height),
            ]
        )
        faces.append((first, first + 1, first + 2, first + 3))

    if len(faces) != LIGHTS_PER_NIGHT[night - 1]:
        raise RuntimeError(f"{name}: {len(faces)} lights, not {LIGHTS_PER_NIGHT[night - 1]}")
    check_reach(name, vertices)

    model = common.create_mesh_object(name, vertices, faces)
    common.piece_project_uvs(model.data, [PALE] * len(faces), UV_UNITS_PER_METRE)
    common.assign_textured_material(model, "lantern", "lantern.png", "lantern_normal.png")
    common.export_obj(name)


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []
    # For every face, in the order of `faces`: PALE or DARK.
    tones = []
    # Every window and lamp head: (x, y of the wall behind it, height of its lower edge,
    # width, height, the night that lights it), in the numbers of the sketch.
    lights = []

    add_ridge(vertices, faces, tones)
    for house in BACK_HOUSES + FRONT_HOUSES:
        add_house(vertices, faces, tones, lights, house)
    add_house(vertices, faces, tones, lights, CHAPEL)
    add_tower(vertices, faces, tones, lights)
    add_well(vertices, faces, tones)
    add_lamps(vertices, faces, tones, lights)
    check_reach(NAME, vertices)

    model = common.create_mesh_object(NAME, vertices, faces)
    common.piece_project_uvs(model.data, tones, UV_UNITS_PER_METRE)
    common.assign_textured_material(model, "lantern", "lantern.png", "lantern_normal.png")
    common.export_obj(NAME)

    if shots:
        # From the eye, with the lens of the review camera, and from nearer. The second
        # camera stays below the lane like the eye: from above, the faces that are left
        # out would be missing.
        common.render_review_shots(
            NAME,
            target=(0.0, LANE_DISTANCE, LANE_HEIGHT + 4.0),
            camera_positions=[(0.0, 0.0, 0.0), (-12.0, LANE_DISTANCE - 60.0, LANE_HEIGHT - 4.0)],
        )

    for night in range(1, NIGHT_COUNT + 1):
        export_lights(night, lights)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
