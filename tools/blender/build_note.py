# Builds the model note: a sheet of old paper pinned to a wall, with lines of ink on it.
# Output: assets/models/note.obj and note.mtl.
# See docs/guides/blender.md
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_note.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import os
import sys

# Blender does not add the folder of the script to the module search path, so the helper
# module next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common

NAME = "note"

# All sizes are in metres. The geometry is written in Blender space: X is the width, Z is
# the height and Y is the depth. The exporter maps a Blender point (x, y, z) to the game
# point (x, z, -y), so the game +Z axis (out of the wall, towards the player) is the
# Blender -Y axis.
# The origin of the model is the point of the wall face behind the middle of the sheet:
# the wall face is the game plane z = 0, and the sheet reaches equally far to the left and
# the right, and up and down.

# The sheet is 0.30 m wide and 0.40 m high. That is inside the box the game tests the
# mouse ray against (0.4 m wide, 0.5 m high and 0.15 m deep, see src/game/Interactables.hpp).
HALF_WIDTH = 0.15
HALF_HEIGHT = 0.20
# The back of the sheet is 4 mm away from the wall. If both lay in the same plane, the
# depth buffer could not decide which of them is in front, and the sheet would flicker
# against the stone (z-fighting).
BACK_DISTANCE = 0.004
# The front of the sheet is 12 mm away from the wall, so the sheet is 8 mm thick. Real
# paper is much thinner, but then its edge could not be seen from the side.
FRONT_DISTANCE = 0.012

# The four thin edge faces show a strip at the border of the picture. This is the width of
# that strip in UV units: 0.02 of the picture is about 10 of its 512 pixels. The ink lines
# of note_paper.png keep a much larger margin, so the strip is always plain paper.
EDGE_STRIP = 0.02


def note_uvs(mesh):
    """Gives the front of the sheet the whole picture once, and every edge a thin strip.

    The helpers in blender_common repeat a texture every few metres. Here the picture
    must fit the sheet exactly, so the UVs are the position on the sheet as a fraction
    from 0 to 1. The picture is square and the sheet is not, so the picture is shown a
    little narrower than it was painted. The ink lines stay level.
    """
    uv_layer = mesh.uv_layers.new(name="uv")

    # The front and the back of the sheet as Blender Y coordinates (see the note about
    # the minus sign at the top of the file).
    front_y = -FRONT_DISTANCE
    back_y = -BACK_DISTANCE

    for polygon in mesh.polygons:
        normal = polygon.normal

        for loop_index in polygon.loop_indices:
            # A loop is one corner of one face. UVs are stored per loop and not per
            # vertex, because the same vertex needs different UVs on different faces.
            position = mesh.vertices[mesh.loops[loop_index].vertex_index].co

            # Where the corner lies on the sheet, each as a fraction from 0 to 1:
            # `across` from the left edge to the right edge, `upwards` from the bottom
            # edge to the top edge, `depth` from the front of the sheet to its back.
            # Left and right are meant for someone who stands in front of the wall and
            # looks at the sheet. That person looks along Blender +Y (game -Z), so the
            # right hand side is Blender +X, which is also game +X.
            across = (position.x + HALF_WIDTH) / (2.0 * HALF_WIDTH)
            upwards = (position.z + HALF_HEIGHT) / (2.0 * HALF_HEIGHT)
            depth = (position.y - front_y) / (back_y - front_y)

            if normal.y < -0.5:
                # The front, facing Blender -Y: the whole picture. u grows to the right
                # of the viewer and v grows upwards, so the ink lines start at the left
                # and the picture is not a mirror image.
                u = across
                v = upwards
            elif normal.x > 0.5:
                # The right edge: the strip at the right border of the picture. The
                # strip runs along the edge, and its width covers the thickness of the
                # sheet.
                u = 1.0 - EDGE_STRIP + EDGE_STRIP * depth
                v = upwards
            elif normal.x < -0.5:
                # The left edge: the strip at the left border of the picture.
                u = EDGE_STRIP * (1.0 - depth)
                v = upwards
            elif normal.z > 0.5:
                # The top edge: the strip at the top border of the picture.
                u = across
                v = 1.0 - EDGE_STRIP + EDGE_STRIP * depth
            else:
                # The bottom edge: the strip at the bottom border of the picture.
                u = across
                v = EDGE_STRIP * (1.0 - depth)

            # On every edge the corners at the front of the sheet get the inner side of
            # the strip and the corners at the back get the border of the picture. With
            # this choice u still grows to the right and v still grows upwards for
            # someone who looks at that edge from outside, like on the front. A strip the
            # other way round would be a mirror image, and the normal map would then
            # light it from the wrong side.
            uv_layer.data[loop_index].uv = (u, v)


def build(shots):
    common.reset_scene()

    vertices = []
    faces = []

    # The sheet: one thin box. Its back, the side facing Blender +Y, faces the wall from
    # 4 mm away and is never seen.
    common.add_box(
        vertices,
        faces,
        (-HALF_WIDTH, -FRONT_DISTANCE, -HALF_HEIGHT),
        (HALF_WIDTH, -BACK_DISTANCE, HALF_HEIGHT),
        skip=("+y",),
    )

    model = common.create_mesh_object(NAME, vertices, faces)
    note_uvs(model.data)
    common.assign_textured_material(model, "note_paper", "note_paper.png", "note_paper_normal.png")
    common.export_obj(NAME)

    if shots:
        # Two views from in front of the wall (the front is Blender -Y): straight at the
        # sheet, to read the ink lines, and from the right side, to see the edge.
        common.render_review_shots(
            NAME,
            target=(0.0, 0.0, 0.0),
            camera_positions=[(0.0, -0.9, 0.0), (0.6, -0.5, 0.25)],
        )


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
