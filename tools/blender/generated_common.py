# Shared helpers of the scripts that build a model from a source a tool generated (see
# tools/blender/sources/): import the GLB with its two pictures, find islands and holes of
# the surface, and shrink the pictures for the game. The shade was the first such model,
# and these helpers came out of build_shade.py.
import os

import bpy
import numpy as np

import make_textures

SOURCES_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sources")


def import_source(file_name):
    """Imports a GLB from the sources folder and returns its mesh object and its two
    pictures as arrays.

    The pictures are rows from the bottom, as Blender stores them, with the channels from
    0 to 1 exactly as the file has them (sRGB for the colour, the directions for the
    normal map).
    """
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=os.path.join(SOURCES_DIR, file_name))
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


def neighbours(face):
    return [other for edge in face.edges for other in edge.link_faces if other is not face]


def islands(mesh):
    """The groups of faces that share edges with each other, largest first.

    Groups of the same size keep the order of their first face, so the same mesh gives
    the same list on every run.
    """
    mesh.faces.index_update()
    seen = set()
    found = []
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
        found.append(island)
    # sorted() is stable, so equal sizes stay in the order they were found.
    return sorted(found, key=len, reverse=True)


def largest_island(mesh):
    """The faces of the largest group of faces that share edges with each other."""
    return islands(mesh)[0]


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


def shrink(picture, size):
    """The picture at size by size: every texel the average of a square of the source.

    The source must be a whole multiple of size.
    """
    factor = picture.shape[0] // size
    return picture.reshape(size, factor, size, factor, 3).mean(axis=(1, 3))


def save_normal_map(normal, size, file_name, relief=1.0):
    """Shrinks a normal map to size by size and writes it.

    The directions are averaged as directions: from 0..1 to -1..1, then to length 1 again.
    relief scales the slopes of the map: 2 makes every bump and groove twice as steep, for
    a source whose map is too faint to show in the light of the game.
    """
    directions = shrink(normal, size) * 2.0 - 1.0
    directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    if relief != 1.0:
        directions[..., :2] *= relief / np.maximum(directions[..., 2:], 0.05)
        directions[..., 2] = 1.0
        directions /= np.linalg.norm(directions, axis=-1, keepdims=True)
    make_textures.save_png(directions * 0.5 + 0.5, file_name)
