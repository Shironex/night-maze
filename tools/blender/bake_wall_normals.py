# Bakes the normal maps of the plain and the mossy wall from the wall slabs Tripo generated
# from their pictures: tools/blender/sources/wall_stone_tripo.glb and wall_mossy_tripo.glb.
# Each slab is a block of real stones about 1 m wide whose front shows the picture it was
# made from (wall_stone.png, wall_mossy.png in the same folder). The game's walls stay flat
# boxes, so their stones get the relief of the slab through a normal map.
#
# Output, in tools/blender/sources: wall_stone_normal_baked.png and
# wall_mossy_normal_baked.png, 1024 x 1024, one texel for each pixel of the source picture.
# They are committed, and make_textures.py builds the game's wall pictures from them. The
# bake and the fit below take minutes, which make_all.py should not pay on every run.
#
# Run from the repository root (only when a slab or its picture changes):
#   blender --background --factory-startup --python tools/blender/bake_wall_normals.py
#
# The steps for each slab:
# 1. Cycles bakes the normals of the slab's front (its geometry and its own normal map) onto
#    a flat panel in front of it, in the tangent space of the panel: the OpenGL convention
#    of make_textures.py, +Y up the picture.
# 2. Where a ray missed the stones or hit the slab's rim, the bake is filled from its
#    neighbours. The slab bows as a whole, and that slow lean is taken out: only the relief
#    of the stones is kept.
# 3. The slab does not show the picture at its exact size: Tripo scaled it, moved it and
#    bent it a little. The bake is lined up with the picture: first a scale and an offset,
#    found by comparing the outlines of the stones (the edges of the slab's colour against
#    the edges of the picture), then a smooth field of small shifts, found piece by piece
#    in the same way. The script prints how far the outlines of the relief lie from the
#    outlines of the picture after that.
import os
import sys

import bmesh
import bpy
import numpy as np

# Blender does not add the folder of the script to the module search path, so the helper
# modules next to this file would not be found without this line.
sys.path.append(os.path.dirname(os.path.abspath(__file__)))

import generated_common as generated
import make_textures

# The slabs, the pictures they were made from, and the files written.
SLABS = (
    ("wall_stone_tripo.glb", "wall_stone.png", "wall_stone_normal_baked.png"),
    ("wall_mossy_tripo.glb", "wall_mossy.png", "wall_mossy_normal_baked.png"),
)

# The size of the source pictures and of the files written.
PICTURE_SIZE = 1024
# The bake has more texels than the picture: the slab is a little larger than the picture
# and the bake is resampled when it is lined up.
BAKE_SIZE = 2048
# Cycles samples every texel this many times with the same seed, so a bake gives the same
# numbers on every run. More samples only smooth the edges of the stones a little.
BAKE_SAMPLES = 8

# The rays start this far in front of the panel and look this far towards the slab. The
# stones stand up to 0.25 m out of the middle of the slab.
CAGE_EXTRUSION = 0.6
MAX_RAY_DISTANCE = 1.2

# A texel whose normal leans more than this away from the panel (its z below this) did
# not hit the face of a stone, and is filled from its neighbours.
LOWEST_NORMAL_Z = 0.35

# The lean of the slab as a whole: the slope of the bake averaged over this many texels
# of the bake to each side. Stones are about 100 texels long, so this keeps them.
BOW_RADIUS = 96

# The scales of the picture on the slab that are tried, as a share of the slab's width.
FIT_SCALES = np.arange(0.85, 1.2501, 0.01)
# The pieces in which the small shifts are measured, as (distance between pieces, size of
# a piece, largest shift) in pixels of the picture. Each round starts from the last.
WARP_ROUNDS = ((64, 128, 24), (32, 96, 8), (32, 64, 4))

LUMINANCE_WEIGHTS = np.array((0.2126, 0.7152, 0.0722))


def make_panel(low, high):
    """A square face over the front of the slab, facing -Y, with u to the right (+X) and v
    up (+Z), like the box_project_uvs of a wall face seen from outside."""
    mesh = bpy.data.meshes.new("panel")
    surface = bmesh.new()
    corners = [(low[0], 0.0, low[2]), (high[0], 0.0, low[2]), (high[0], 0.0, high[2]),
               (low[0], 0.0, high[2])]
    face = surface.faces.new([surface.verts.new(corner) for corner in corners])
    uv_layer = surface.loops.layers.uv.new("uv")
    for loop, uv in zip(face.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
        loop[uv_layer].uv = uv
    surface.to_mesh(mesh)
    panel = bpy.data.objects.new("panel", mesh)
    bpy.context.scene.collection.objects.link(panel)
    return panel


def bake(slab):
    """Returns the colour and the normals of the slab's front, baked onto a panel over its
    bounding box, as BAKE_SIZE x BAKE_SIZE x 3 arrays, rows from the bottom. The colour
    is sRGB from 0 to 1, the normals are directions with components from -1 to 1."""
    corners = np.array([vertex.co[:] for vertex in slab.data.vertices])
    panel = make_panel(corners.min(axis=0), corners.max(axis=0))
    assert panel.data.polygons[0].normal.y < -0.99, "the panel must face -Y"

    material = bpy.data.materials.new("panel")
    material.use_nodes = True
    panel.data.materials.append(material)
    image = bpy.data.images.new("bake", BAKE_SIZE, BAKE_SIZE, alpha=False, float_buffer=True)
    image.colorspace_settings.name = "Non-Color"
    node = material.node_tree.nodes.new("ShaderNodeTexImage")
    node.image = image
    material.node_tree.nodes.active = node

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.cycles.samples = BAKE_SAMPLES
    scene.cycles.seed = 0
    for item in bpy.data.objects:
        item.select_set(False)
    slab.select_set(True)
    panel.select_set(True)
    bpy.context.view_layer.objects.active = panel

    def run(kind, **options):
        bpy.ops.object.bake(
            type=kind,
            use_selected_to_active=True,
            cage_extrusion=CAGE_EXTRUSION,
            max_ray_distance=MAX_RAY_DISTANCE,
            margin=0,
            **options,
        )
        pixels = np.empty(BAKE_SIZE * BAKE_SIZE * 4, dtype=np.float32)
        image.pixels.foreach_get(pixels)
        return pixels.reshape(BAKE_SIZE, BAKE_SIZE, 4)[..., :3].astype(np.float64)

    light = run("DIFFUSE", pass_filter={"COLOR"})
    normals = run("NORMAL", normal_space="TANGENT") * 2.0 - 1.0
    return make_textures.linear_to_srgb(np.clip(light, 0.0, 1.0)), normals


def box_blur(values, radius):
    """The average over a square of 2 * radius + 1 texels, with the border repeated
    outward (the bake does not tile, so the opposite edge would be wrong here)."""
    for axis in (0, 1):
        padding = [(0, 0)] * values.ndim
        padding[axis] = (radius + 1, radius)
        total = np.cumsum(np.pad(values, padding, mode="edge"), axis=axis)
        count = values.shape[axis]
        values = (np.take(total, range(2 * radius + 1, 2 * radius + 1 + count), axis=axis)
                  - np.take(total, range(0, count), axis=axis)) / (2 * radius + 1)
    return values


def relief_slopes(normals):
    """The slopes of the relief (dheight/dx, dheight/dy) from baked normals, with the
    texels that missed the stones filled from their neighbours and the bow taken out, and
    a mask that is 1 where the bake hit the stones and 0 where it was filled."""
    good = normals[..., 2] > LOWEST_NORMAL_Z
    slopes = -normals[..., :2] / np.maximum(normals[..., 2:], LOWEST_NORMAL_Z)
    slopes[~good] = 0.0
    # The average of the good neighbours, over a square that grows until it reaches some.
    weight = good.astype(np.float64)
    for radius in (2, 4, 8, 16, 32):
        filled = box_blur(slopes * weight[..., None], radius)
        share = box_blur(weight, radius)
        missing = (weight == 0.0) & (share > 0.0)
        slopes[missing] = filled[missing] / share[missing][..., None]
        weight[missing] = 1.0
    print("filled", int((~good).sum()), "texels of the bake")
    return slopes - box_blur(slopes, BOW_RADIUS), good.astype(np.float64)


def edges(picture):
    """The outlines of the stones: the size of the slope of the softened brightness,
    scaled to an average of 0 and a spread of 1, so two pictures can be compared."""
    brightness = box_blur(picture @ LUMINANCE_WEIGHTS, 2)
    return normalised(slope_size(brightness))


def slope_size(height):
    slope_x = np.zeros_like(height)
    slope_y = np.zeros_like(height)
    slope_x[:, 1:-1] = height[:, 2:] - height[:, :-2]
    slope_y[1:-1] = height[2:] - height[:-2]
    return np.hypot(slope_x, slope_y)


def normalised(values):
    return (values - values.mean()) / values.std()


def sample(values, rows, columns):
    """values at the (fractional) rows and columns, interpolated between the four
    nearest texels. rows and columns are arrays of the same shape."""
    top = values.shape[0] - 2
    right = values.shape[1] - 2
    row = np.clip(np.floor(rows).astype(int), 0, top)
    column = np.clip(np.floor(columns).astype(int), 0, right)
    down = np.clip(rows - row, 0.0, 1.0)
    across = np.clip(columns - column, 0.0, 1.0)
    if values.ndim == 3:
        down = down[..., None]
        across = across[..., None]
    return (values[row, column] * (1 - down) * (1 - across)
            + values[row + 1, column] * down * (1 - across)
            + values[row, column + 1] * (1 - down) * across
            + values[row + 1, column + 1] * down * across)


def best_shift(target, moved, largest):
    """The shift (rows, columns) that moves moved onto target best, up to largest
    pixels each way, by cross-correlation."""
    size = target.shape[0]
    match = np.real(np.fft.ifft2(np.fft.fft2(target) * np.conj(np.fft.fft2(moved))))
    match = np.fft.fftshift(match)
    middle = size // 2
    window = match[middle - largest:middle + largest + 1, middle - largest:middle + largest + 1]
    row, column = divmod(int(np.argmax(window)), 2 * largest + 1)
    return row - largest, column - largest


def fit_scale_and_offset(picture_edges, slab_edges):
    """Where the bake lies on the picture: (scale_y, scale_x, offset_y, offset_x), the
    size of the whole bake and the position of its first texel, in pixels of the picture.

    The middle of the bake is shrunk to each of the scales in turn and slid over the
    picture (cross-correlation through the FFT). The best match over all wins. This runs
    on pictures a quarter of the size, then once more at full size around the winner.
    """
    def search(picture, slab, scales_y, scales_x):
        size = picture.shape[0]
        bake_size = slab.shape[0]
        low = int(0.15 * bake_size)
        high = int(0.85 * bake_size)
        picture_spectrum = np.fft.fft2(picture)
        best = None
        for scale_y in scales_y:
            for scale_x in scales_x:
                rows = np.arange(low, high, bake_size / size / scale_y)
                columns = np.arange(low, high, bake_size / size / scale_x)
                grid_rows, grid_columns = np.meshgrid(rows, columns, indexing="ij")
                piece = normalised(sample(slab, grid_rows, grid_columns))
                padded = np.zeros((size, size))
                padded[:piece.shape[0], :piece.shape[1]] = piece
                match = np.real(np.fft.ifft2(picture_spectrum * np.conj(np.fft.fft2(padded))))
                index = int(np.argmax(match))
                if best is None or match.flat[index] > best[0]:
                    shift_y, shift_x = divmod(index, size)
                    offset_y = shift_y - low * size * scale_y / bake_size
                    offset_x = shift_x - low * size * scale_x / bake_size
                    best = (match.flat[index], scale_y, scale_x, offset_y, offset_x)
        return best[1:]

    quarter = PICTURE_SIZE // 4
    small_picture = picture_edges.reshape(quarter, 4, quarter, 4).mean(axis=(1, 3))
    small_slab = slab_edges.reshape(BAKE_SIZE // 4, 4, BAKE_SIZE // 4, 4).mean(axis=(1, 3))
    scale_y, scale_x, _, _ = search(small_picture, small_slab, FIT_SCALES, FIT_SCALES)
    fine = np.arange(-0.01, 0.0101, 0.0025)
    scale_y, scale_x, offset_y, offset_x = search(
        picture_edges, slab_edges, scale_y + fine, scale_x + fine)

    # The plain picture repeats every half of its height, so a match one half higher or
    # lower is as good. The one is kept where the bake covers the whole picture.
    half = PICTURE_SIZE / 2
    while offset_y > 0.0:
        offset_y -= half
    while offset_y + PICTURE_SIZE * scale_y < PICTURE_SIZE:
        offset_y += half
    return scale_y, scale_x, offset_y, offset_x


def patch_shifts(picture_edges, moved_edges, spacing, size, largest):
    """The shift of moved_edges onto picture_edges, measured in square pieces of the
    given size whose middles lie spacing apart. Returns the middles and the shifts."""
    half = size // 2
    middles = np.arange(half, PICTURE_SIZE - half + 1, spacing)
    shifts = np.zeros((len(middles), len(middles), 2))
    for i, row in enumerate(middles):
        for j, column in enumerate(middles):
            shifts[i, j] = best_shift(
                picture_edges[row - half:row + half, column - half:column + half],
                moved_edges[row - half:row + half, column - half:column + half],
                largest,
            )
    return middles, shifts


def median_of_neighbours(shifts):
    """Every shift replaced by the median of itself and its 8 neighbours: one piece that
    matched the wrong stone does not bend the field."""
    padded = np.pad(shifts, ((1, 1), (1, 1), (0, 0)), mode="edge")
    rows, columns = shifts.shape[:2]
    around = [padded[i:i + rows, j:j + columns] for i in range(3) for j in range(3)]
    return np.median(np.stack(around), axis=0)


def line_up(picture, slab_colour, slab_slopes, slab_hit):
    """Returns the slopes of the slab and its mask of hits (see relief_slopes) resampled
    onto the pixels of the picture. Prints the shifts that are left between the outlines
    of the relief and of the picture."""
    picture_edges = edges(picture)
    slab_edges = edges(slab_colour)
    scale_y, scale_x, offset_y, offset_x = fit_scale_and_offset(picture_edges, slab_edges)
    print("scale", round(scale_y, 4), round(scale_x, 4),
          "offset", round(offset_y, 1), round(offset_x, 1))

    # Where every pixel of the picture lies in the bake.
    pixels = np.arange(PICTURE_SIZE, dtype=np.float64)
    rows = (pixels - offset_y) * BAKE_SIZE / (PICTURE_SIZE * scale_y)
    columns = (pixels - offset_x) * BAKE_SIZE / (PICTURE_SIZE * scale_x)
    rows, columns = np.meshgrid(rows, columns, indexing="ij")

    # The small shifts, round after round, each a field over the whole picture.
    for spacing, size, largest in WARP_ROUNDS:
        moved = sample(slab_edges, rows, columns)
        middles, shifts = patch_shifts(picture_edges, moved, spacing, size, largest)
        shifts = median_of_neighbours(shifts)
        steps = np.interp(pixels, middles, np.arange(len(middles)))
        grid_rows, grid_columns = np.meshgrid(steps, steps, indexing="ij")
        field = sample(shifts, grid_rows, grid_columns)
        rows = rows - field[..., 0] * BAKE_SIZE / (PICTURE_SIZE * scale_y)
        columns = columns - field[..., 1] * BAKE_SIZE / (PICTURE_SIZE * scale_x)

    slopes = sample(slab_slopes, rows, columns)
    hit = sample(slab_hit, rows, columns)

    # How far the relief is from the picture now: its outlines (where the slope is large)
    # against the outlines of the picture, over the whole picture (make_textures.py shows
    # all of it but a thin border).
    relief_edges = normalised(np.hypot(slopes[..., 0], slopes[..., 1]))
    middles, left = patch_shifts(picture_edges, relief_edges, 64, 128, 16)
    distance = np.hypot(left[..., 0], left[..., 1])
    worst_row, worst_column = np.unravel_index(int(np.argmax(distance)), distance.shape)
    print("relief against picture, pixels: median", np.median(distance),
          "90th percentile", np.percentile(distance, 90), "largest", distance.max(),
          "around the pixel", (middles[worst_column], middles[worst_row]))
    return slopes, hit


def build():
    for slab_file, picture_file, output_file in SLABS:
        print(slab_file)
        picture = make_textures.load_source_picture(picture_file)
        slab, _, _ = generated.import_source(slab_file)
        colour, normals = bake(slab)
        slopes, hit = line_up(picture, colour, *relief_slopes(normals))

        # Where the slab has a hole, the filled bake is flat. The relief made from the
        # picture goes there instead, scaled to lean as much as the baked relief.
        made = make_textures.picture_slopes(picture)
        sure = hit > 0.99
        scale = (np.median(np.hypot(slopes[sure, 0], slopes[sure, 1]))
                 / np.median(np.hypot(made[sure, 0], made[sure, 1])))
        hit = make_textures.smooth_step(np.clip(box_blur(hit, 4) * 2.0 - 1.0, 0.0, 1.0))
        slopes = hit[..., None] * slopes + (1.0 - hit[..., None]) * scale * made
        normal = np.concatenate([-slopes, np.ones(slopes.shape[:2] + (1,))], axis=-1)
        normal = normal / np.linalg.norm(normal, axis=-1, keepdims=True)
        make_textures.save_png(normal * 0.5 + 0.5, output_file, generated.SOURCES_DIR)


if __name__ == "__main__":
    build()
