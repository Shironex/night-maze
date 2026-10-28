# Builds the generated pieces of the land around the maze: the trees, the houses, the rick
# and the landmarks (src/game/Land.hpp). None of them is built in this script. Their sources
# are models that Tripo generated, tools/blender/sources/<source>_tripo.glb, one material
# each with a colour picture and a normal map. This script scales each to its size in the
# land, stands it on its origin, cuts it down to the triangles the land can pay for and
# writes the game files, the same bytes on every run.
#
# A piece can have two versions. The near one stands in the near band of the land, 1.5 to
# 14 m past the border wall, and is seen from a few metres: assets/models/land_<name>.obj.
# The far one stands in the middle band, 14 to 40 m out, where only what is high reads:
# assets/models/land_<name>_far.obj, about 1,500 triangles a tree and 3,000 a landmark. Both
# wear the same pictures, assets/textures/land_<picture>.png and land_<picture>_normal.png:
# the decimation keeps the UVs.
#
# A house also gets the places of its windows, found by rays (find_windows), in
# assets/models/land_<name>_windows.txt. The game lays an amber quad on a window when its
# night has been won (game::dealLandWindows).
#
# Some pieces are stand-ins until their own model is generated: the source named in PIECES
# is then another model, scaled to the size the real one should have. The list is in
# docs/decisions/land-around-the-maze.md. A real source drops in by changing its line.
#
# Run from the repository root:
#   blender --background --factory-startup --python tools/blender/build_land_pieces.py
# Add "-- --shots" at the end to also write review renders to the temporary folder.
import math
import os
import sys
from dataclasses import dataclass

import bmesh
import bpy
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import blender_common as common
import generated_common as generated
import make_textures


@dataclass(frozen=True)
class Piece:
    # The name of the game files: land_<name>.obj, land_<name>_far.obj.
    name: str
    # The source in tools/blender/sources.
    source: str
    # The height of the piece above its origin, in metres.
    height: float
    # Triangles of the near version and of the far version. 0: that version is not built.
    near: int
    far: int
    # The pictures the piece wears, land_<picture>.png, and their size in pixels. Two
    # pieces made from one source share them.
    picture: str
    picture_size: int
    # How far the piece sinks into the ground, in metres. The trees stand on a disc of
    # earth and roots, which goes into the hill; a house stands on its floor.
    sink: float = 0.1
    # Whether the windows of the piece are looked for.
    windows: bool = False
    # How many copies of the source stand together in one piece (a copse), with their
    # offsets and turns in COPSE_PLACES.
    copies: int = 1


# The near band holds the trees, the farmhouse with its shed and barn, and a rick. The
# middle band holds bare trees, copses and cottages: the trees in leaf lose their leaves to
# shards below about 4,000 triangles, so they have no far version. The landmarks stand on its crests.
#
# Stand-ins (see the comment at the top): the oak is the rowan of the source set (a
# full-leaf oak is coming), the rick is the peat stack, the copse is three dead oaks, the
# standing stone is the granite tor, the dovecote is the moor cottage, the watch hut is the
# woodshed, the mill is the stone cottage and the church tower the chapel ruin.
PIECES = (
    Piece("oak", "rowan_tree_tripo.glb", 10.0, 6000, 0, "rowan", 512, sink=0.3),
    Piece("hawthorn", "hawthorn_tree_tripo.glb", 6.0, 3500, 0, "hawthorn", 512, sink=0.25),
    Piece("dead_oak", "dead_oak_tripo.glb", 9.0, 3000, 1200, "dead_oak", 512, sink=0.3),
    Piece("farmhouse", "stone_cottage_tripo.glb", 7.0, 4000, 1500, "stone_cottage", 1024,
          windows=True),
    Piece("cottage", "moor_cottage_tripo.glb", 5.5, 0, 1500, "moor_cottage", 1024,
          windows=True),
    Piece("barn", "ruined_barn_tripo.glb", 6.5, 3500, 0, "ruined_barn", 1024),
    Piece("woodshed", "log_woodshed_tripo.glb", 3.2, 2000, 0, "log_woodshed", 512),
    Piece("rick", "peat_stack_tripo.glb", 4.0, 1500, 0, "peat_stack", 512, sink=0.15),
    # The decimation takes the tips of the tallest twigs: the copse ends 8.4 m high.
    Piece("copse", "dead_oak_tripo.glb", 9.0, 0, 2400, "dead_oak", 512, sink=0.3, copies=3),
    Piece("standing_stone", "granite_tor_tripo.glb", 5.5, 0, 3000, "granite_tor", 1024),
    Piece("dovecote", "moor_cottage_tripo.glb", 6.5, 0, 3000, "moor_cottage", 1024),
    Piece("chapel_gable", "chapel_ruin_tripo.glb", 7.0, 0, 3000, "chapel_ruin", 1024,
          windows=True),
    Piece("watch_hut", "log_woodshed_tripo.glb", 5.0, 0, 3000, "log_woodshed", 512),
    Piece("cairn", "stone_cairn_tripo.glb", 5.0, 0, 3000, "stone_cairn", 1024),
    # The far band: drawn around the eye on the ridge, like the village (game::FarSilhouette),
    # and scaled up there (SILHOUETTE_PLACES in src/game/Land.cpp). The windows are found at
    # this size, where they are the size of the windows of a house.
    Piece("mill", "stone_cottage_tripo.glb", 7.0, 0, 1500, "stone_cottage", 1024,
          windows=True),
    Piece("church_tower", "chapel_ruin_tripo.glb", 7.0, 0, 3000, "chapel_ruin", 1024,
          windows=True),
)

# The three trees of a copse, as (x, y, turn in degrees, scale) of each copy, in metres of
# the finished piece: a clump about 6 m across.
COPSE_PLACES = ((0.0, 0.0, 0.0, 1.0), (2.6, 1.2, 140.0, 0.8), (-1.9, 2.1, 250.0, 0.7))

# How often the decimation runs at most, and how far above the target it may stop.
DECIMATE_RUNS = 4
DECIMATE_TOLERANCE = 0.05

# Two corners of the source closer than this are one corner (see build_stone_sheep.py).
MERGE_DISTANCE = 0.000001

# The windows. Rays go level into each of the four sides of a house, every WINDOW_STEP
# metres across the side and up from WINDOW_LOWEST to WINDOW_HIGHEST of its height. A ray
# that meets a face of that side (its normal within WINDOW_FACING of the side) on a texel
# that is dark (every channel under WINDOW_DARK) or lamp yellow (red over WINDOW_WARM_RED
# and over blue by WINDOW_WARM_SPREAD, the moor cottage has a lit window painted on) has met
# a window. Neighbouring hits are one window, kept when it is WINDOW_MIN to WINDOW_MAX
# metres wide and high and fills at least WINDOW_FILL of its rectangle. A door is taller
# than WINDOW_MAX and is left out.
WINDOW_STEP = 0.04
WINDOW_LOWEST = 0.12
WINDOW_HIGHEST = 0.75
WINDOW_FACING = 0.8
WINDOW_DARK = 0.16
WINDOW_WARM_RED = 0.5
WINDOW_WARM_SPREAD = 0.25
WINDOW_MIN = 0.25
WINDOW_MAX = 1.5
WINDOW_FILL = 0.4
# A window light stands this far in front of its wall, so the depth test never lets the
# wall show through it.
WINDOW_OFFSET = 0.04
# A house that shows no window to the rays (the chapel has empty arches) gets one light in
# the middle of its front, at this part of its height, as large as WINDOW_FALLBACK_SIZE.
WINDOW_FALLBACK_HEIGHT = 0.45
WINDOW_FALLBACK_SIZE = (0.8, 1.2)


def cleaned(model):
    """The surface of the source as a bmesh in metres: the largest island and every piece
    that hangs on it, standing on the origin at the middle of its foot."""
    mesh = bmesh.new()
    mesh.from_mesh(model.data)
    bmesh.ops.transform(mesh, matrix=model.matrix_world, verts=mesh.verts)
    bmesh.ops.remove_doubles(mesh, verts=mesh.verts, dist=MERGE_DISTANCE)
    return mesh


def stand(mesh, piece):
    """Scales the mesh to the height of the piece and puts its foot on the origin, sunk by
    piece.sink."""
    zs = [corner.co.z for corner in mesh.verts]
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    scale = piece.height / (max(zs) - min(zs))
    middle = Vector(((min(xs) + max(xs)) / 2.0, (min(ys) + max(ys)) / 2.0, min(zs)))
    for corner in mesh.verts:
        corner.co = (corner.co - middle) * scale - Vector((0.0, 0.0, piece.sink))


def decimated(mesh, triangles):
    """A copy of the mesh with about this many triangles, by collapsing edges. The UVs
    stay, so the copy wears the pictures of the source.

    One run of the modifier stops early on some trees: an edge whose collapse would fold
    a face over is skipped, and the thin leaves have many of them. A second run, on the
    result of the first, finds new edges to collapse, so it runs up to DECIMATE_RUNS
    times until the count is within DECIMATE_TOLERANCE of the target.
    """
    common.reset_scene()
    data = bpy.data.meshes.new("decimate")
    mesh.to_mesh(data)
    model = bpy.data.objects.new("decimate", data)
    bpy.context.scene.collection.objects.link(model)
    bpy.context.view_layer.objects.active = model
    for _ in range(DECIMATE_RUNS):
        if len(model.data.polygons) <= triangles * (1.0 + DECIMATE_TOLERANCE):
            break
        modifier = model.modifiers.new("decimate", "DECIMATE")
        modifier.decimate_type = "COLLAPSE"
        modifier.ratio = triangles / len(model.data.polygons)
        modifier.use_collapse_triangulate = True
        bpy.ops.object.modifier_apply(modifier="decimate")
    result = bmesh.new()
    result.from_mesh(model.data)
    bmesh.ops.triangulate(result, faces=result.faces)
    common.reset_scene()
    return result


def copse(mesh):
    """Three copies of the mesh, turned and scaled, as one mesh (COPSE_PLACES)."""
    result = bmesh.new()
    data = bpy.data.meshes.new("copy")
    mesh.to_mesh(data)
    for x, y, turn, scale in COPSE_PLACES:
        placed = (Matrix.Translation((x, y, 0.0)) @ Matrix.Rotation(math.radians(turn), 4, "Z")
                  @ Matrix.Scale(scale, 4))
        copy = data.copy()
        copy.transform(placed)
        result.from_mesh(copy)
    return result


def write_model(mesh, file_name, picture):
    common.reset_scene()
    data = bpy.data.meshes.new(file_name)
    mesh.to_mesh(data)
    # Bark, leaves and stone are made of many small faces: smooth normals, like the sheep.
    data.shade_smooth()
    # The normal-map node of the material names the UV map "uv".
    data.uv_layers[0].name = "uv"
    model = bpy.data.objects.new(file_name, data)
    bpy.context.scene.collection.objects.link(model)
    common.assign_textured_material(model, file_name, f"land_{picture}.png",
                                    f"land_{picture}_normal.png")
    common.export_obj(file_name)
    # What MODEL_SHAPES in src/game/Land.cpp needs: the height and half of the wider side
    # on the ground (a test checks the height against the file).
    xs = [v.co.x for v in mesh.verts]
    ys = [v.co.y for v in mesh.verts]
    zs = [v.co.z for v in mesh.verts]
    half_width = max(max(xs) - min(xs), max(ys) - min(ys)) / 2.0
    print(f"{file_name}: {len(mesh.faces)} triangles, {max(zs) - min(zs):.2f} m high, "
          f"half width {half_width:.2f} m")


def is_window_texel(texel):
    red, _, blue = texel
    dark = texel.max() < WINDOW_DARK
    warm = red > WINDOW_WARM_RED and red - blue > WINDOW_WARM_SPREAD
    return dark or warm


def windows_on_side(tree, mesh, colour, uv_layer, side, piece):
    """The windows one side of the house shows to level rays that come from far out along
    side (a unit vector, level)."""
    xs = [corner.co.x for corner in mesh.verts]
    ys = [corner.co.y for corner in mesh.verts]
    across = Vector((-side.y, side.x, 0.0))
    reach = max(max(xs) - min(xs), max(ys) - min(ys))
    columns = int(reach / WINDOW_STEP) + 1
    rows = int((WINDOW_HIGHEST - WINDOW_LOWEST) * piece.height / WINDOW_STEP) + 1
    hits = {}
    for column in range(columns):
        for row in range(rows):
            offset = -reach / 2.0 + column * WINDOW_STEP
            height = WINDOW_LOWEST * piece.height + row * WINDOW_STEP
            start = side * (reach * 2.0) + across * offset + Vector((0.0, 0.0, height))
            point, normal, number, _ = tree.ray_cast(start, -side)
            if point is None or normal.dot(side) < WINDOW_FACING:
                continue
            face = mesh.faces[number]
            if is_window_texel(generated.texel_at(colour, face, uv_layer)):
                hits[(column, row)] = point
    # Groups of neighbouring hits, each found once, in the order of the grid.
    windows = []
    seen = set()
    for key in sorted(hits):
        if key in seen:
            continue
        group = [key]
        seen.add(key)
        for column, row in group:
            for other in ((column + 1, row), (column - 1, row), (column, row + 1),
                          (column, row - 1)):
                if other in hits and other not in seen:
                    seen.add(other)
                    group.append(other)
        columns_in = max(c for c, _ in group) - min(c for c, _ in group) + 1
        rows_in = max(r for _, r in group) - min(r for _, r in group) + 1
        width = columns_in * WINDOW_STEP
        height = rows_in * WINDOW_STEP
        if not (WINDOW_MIN <= width <= WINDOW_MAX and WINDOW_MIN <= height <= WINDOW_MAX):
            continue
        if len(group) < WINDOW_FILL * columns_in * rows_in:
            continue
        middle = sum((hits[key] for key in group), Vector()) / len(group)
        windows.append((middle + side * WINDOW_OFFSET, side, width, height))
    return windows


def find_windows(mesh, colour, piece):
    """Every window of the house, as (middle, outward normal, width, height) in Blender
    space, the windows of the four sides in the order +X, +Y, -X, -Y."""
    mesh.faces.ensure_lookup_table()
    uv_layer = mesh.loops.layers.uv.active
    tree = BVHTree.FromBMesh(mesh)
    windows = []
    for side in (Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((-1, 0, 0)), Vector((0, -1, 0))):
        windows += windows_on_side(tree, mesh, colour, uv_layer, side, piece)
    if not windows:
        # The front is -Y in Blender, the side the exporter turns to +Z of the game.
        ys = [corner.co.y for corner in mesh.verts]
        side = Vector((0, -1, 0))
        start = Vector((0.0, min(ys) - 1.0, WINDOW_FALLBACK_HEIGHT * piece.height))
        point, _, _, _ = tree.ray_cast(start, -side)
        middle = point if point is not None else Vector((0.0, min(ys), start.z))
        windows.append((middle + side * WINDOW_OFFSET, side, *WINDOW_FALLBACK_SIZE))
    return windows


def write_windows(windows, name):
    """Writes the windows in the space of the game: one line per window, its middle, its
    outward normal, its width and its height, in metres. Blender (x, y, z) is (x, z, -y)
    in the game, as export_obj turns the models."""
    path = os.path.join(common.MODELS_DIR, f"land_{name}_windows.txt")
    with open(path, "w", encoding="utf-8", newline="\n") as file:
        file.write("# The windows of land_" + name + ".obj, written by "
                   "tools/blender/build_land_pieces.py:\n")
        file.write("# middle x y z, outward normal x y z, width, height, in metres\n")
        for middle, normal, width, height in windows:
            numbers = (middle.x, middle.z, -middle.y, normal.x, normal.z, -normal.y, width,
                       height)
            file.write(" ".join(f"{number:.3f}" for number in numbers) + "\n")
    print(f"land_{name}: {len(windows)} windows")


def write_pictures(colour, normal, piece, written):
    if piece.picture in written:
        return
    written.add(piece.picture)
    make_textures.save_png(generated.shrink(colour, piece.picture_size),
                           f"land_{piece.picture}.png")
    generated.save_normal_map(normal, piece.picture_size, f"land_{piece.picture}_normal.png")


def build_piece(piece, written, shots):
    model, colour, normal = generated.import_source(piece.source)
    mesh = cleaned(model)
    stand(mesh, piece)
    write_pictures(colour, normal, piece, written)
    if piece.windows:
        write_windows(find_windows(mesh, colour, piece), piece.name)
    if piece.copies > 1:
        whole = copse(mesh)
        mesh.free()
        mesh = whole
    if piece.near:
        near = decimated(mesh, piece.near)
        write_model(near, f"land_{piece.name}", piece.picture)
        near.free()
    if piece.far:
        far = decimated(mesh, piece.far)
        write_model(far, f"land_{piece.name}_far", piece.picture)
        far.free()
    mesh.free()
    if shots:
        h = piece.height
        common.render_review_shots(
            f"land_{piece.name}", target=(0.0, 0.0, h * 0.45),
            camera_positions=[(0.0, -h * 2.2, h * 0.6), (h * 1.6, h * 1.6, h * 1.2)])


def build(shots):
    written = set()
    for piece in PIECES:
        build_piece(piece, written, shots)


if __name__ == "__main__":
    build(shots="--shots" in sys.argv)
