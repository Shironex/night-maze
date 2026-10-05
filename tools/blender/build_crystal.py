# Builds the two crystal models the player collects: crystal_a is one tall shard with a
# point at both ends, crystal_b is a cluster of three shards standing on a flat base.
# Output: assets/models/crystal_a.obj, crystal_a.mtl, crystal_b.obj and crystal_b.mtl.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_crystal.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

# All sizes are in metres. The geometry is written in Blender space, where Z is the height.
# After export Z becomes the game Y axis. Both models are 0.5 m tall and their origin is on
# the floor, at the base (for crystal_b at the base of its main shard, not in the middle).

# A shard has six sides, like a quartz crystal.
SIDES = 6

# One repeat of the crystal texture covers 0.5 m, the height of a crystal. With the 2 m of
# the walls a facet 0.1 m wide would only show a few blurred pixels of the picture.
METRES_PER_UV_UNIT = 0.5


def add_ring(vertices, centre, radius, turn):
    """Appends the SIDES corners of a horizontal ring around `centre`.

    The corners go counter clockwise as seen from above. `turn` is the angle of the first
    corner in degrees, so two shards do not have to face the same way.
    """
    centre_x, centre_y, centre_z = centre
    for corner in range(SIDES):
        angle = math.radians(turn + corner * 360.0 / SIDES)
        vertices.append(
            (centre_x + radius * math.cos(angle), centre_y + radius * math.sin(angle), centre_z)
        )


def add_shard(vertices, faces, bottom, bottom_radius, top, top_radius, tip, foot=None, turn=0.0):
    """Appends one shard to the lists `vertices` and `faces`.

    A shard is a column between two horizontal rings with a point on top. `bottom` and
    `top` are the centres of the two rings and `tip` is the point above the upper ring,
    all as (x, y, z). Moving `top` and `tip` sideways makes the shard lean. `foot` is an
    optional second point below the lower ring. Without it the shard gets a flat
    underside.
    """
    # The lower ring gets the indices lower ... lower + 5, the upper ring upper ...
    # upper + 5.
    lower = len(vertices)
    add_ring(vertices, bottom, bottom_radius, turn)
    upper = len(vertices)
    add_ring(vertices, top, top_radius, turn)
    tip_index = len(vertices)
    vertices.append(tip)

    for corner in range(SIDES):
        # The next corner around the ring. After the last one comes the first one again.
        following = (corner + 1) % SIDES
        # One side of the column: a flat four-sided face, because both rings are
        # horizontal and turned by the same angle. The corners go counter clockwise as
        # seen from outside, which makes the normal point outwards.
        faces.append((lower + corner, lower + following, upper + following, upper + corner))
        # One facet of the point on top.
        faces.append((upper + corner, upper + following, tip_index))

    if foot is None:
        # Flat underside: the lower ring in the opposite order, so it faces down.
        faces.append(tuple(lower + corner for corner in reversed(range(SIDES))))
    else:
        foot_index = len(vertices)
        vertices.append(foot)
        for corner in range(SIDES):
            following = (corner + 1) % SIDES
            # One facet of the lower point. It faces down and outwards.
            faces.append((lower + following, lower + corner, foot_index))


def finish(name, vertices, faces, shots):
    """Turns the lists into a textured model, exports it and renders the review shots."""
    model = common.create_mesh_object(name, vertices, faces)
    # The facets are slanted, so box_project_uvs does not fit: every face is projected
    # onto its own plane instead.
    common.face_project_uvs(model.data, METRES_PER_UV_UNIT)
    common.assign_textured_material(model, "crystal", "crystal.png", "crystal_normal.png")
    common.export_obj(name)

    if shots:
        # Two views aimed at the middle of the crystal: from the side, and from above.
        common.render_review_shots(
            name,
            target=(0.0, 0.0, 0.25),
            camera_positions=[(1.0, -1.3, 0.6), (-0.7, -0.9, 1.2)],
        )


def build_crystal_a(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # One upright shard that stands on its lower point. It is a little wider at the top
    # ring than at the bottom ring, and the upper point is longer than the lower one.
    add_shard(
        vertices,
        faces,
        bottom=(0.0, 0.0, 0.12),
        bottom_radius=0.075,
        top=(0.0, 0.0, 0.34),
        top_radius=0.105,
        tip=(0.0, 0.0, 0.5),
        foot=(0.0, 0.0, 0.0),
    )

    finish("crystal_a", vertices, faces, shots)


def build_crystal_b(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # Three shards that grow out of one spot on the floor and lean away from each other.
    # They overlap near the floor, which hides where one ends and the next one starts.
    # The tallest one stands almost upright and reaches the full 0.5 m.
    add_shard(
        vertices,
        faces,
        bottom=(0.0, 0.0, 0.0),
        bottom_radius=0.085,
        top=(0.02, 0.015, 0.36),
        top_radius=0.07,
        tip=(0.03, 0.02, 0.5),
    )
    add_shard(
        vertices,
        faces,
        bottom=(0.1, -0.04, 0.0),
        bottom_radius=0.06,
        top=(0.17, -0.07, 0.22),
        top_radius=0.05,
        tip=(0.2, -0.085, 0.33),
        turn=20.0,
    )
    add_shard(
        vertices,
        faces,
        bottom=(-0.08, 0.06, 0.0),
        bottom_radius=0.05,
        top=(-0.14, 0.11, 0.15),
        top_radius=0.042,
        tip=(-0.165, 0.13, 0.24),
        turn=40.0,
    )

    finish("crystal_b", vertices, faces, shots)


def build(shots):
    build_crystal_a(shots)
    build_crystal_b(shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
