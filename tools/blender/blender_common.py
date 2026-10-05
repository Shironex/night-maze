# Shared helpers of the Blender scripts: scene reset, box building, UV projection, textured
# material, OBJ export with fixed options and review renders.
# See docs/guides/blender.md
#
# Two coordinate systems meet in this file:
#   Blender: right-handed, Z up. All geometry in the scripts is written in this system.
#   Game:    right-handed, Y up, -Z forward. The exporter converts on the way out.
# The exporter maps a Blender point (x, y, z) to the game point (x, z, -y). So in the scripts
# the Blender Z axis is the height, and the Blender Y axis is the game Z axis with the
# opposite sign.
import os
import tempfile

import bpy
from mathutils import Vector

# The scripts live in <repository>/tools/blender, so the repository root is two levels up.
# Building the paths from the location of this file makes the scripts work from any
# working directory.
REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
MODELS_DIR = os.path.join(REPO_ROOT, "assets", "models")
TEXTURES_DIR = os.path.join(REPO_ROOT, "assets", "textures")

# Texel density: one repeat of a texture covers 2 metres on every face of every model.
METRES_PER_UV_UNIT = 2.0

# Size of a review render in pixels.
REVIEW_WIDTH = 960
REVIEW_HEIGHT = 720


def reset_scene():
    """Replaces everything Blender has loaded with an empty scene.

    The factory scene contains a cube, a camera and a light, and they would be exported
    together with the model. It also makes a script independent of whatever a previous
    script left behind when several scripts run in one Blender process (make_all.py).
    """
    bpy.ops.wm.read_factory_settings(use_empty=True)


def add_box(vertices, faces, low, high, skip=()):
    """Appends one axis-aligned box to the lists `vertices` and `faces`.

    `low` and `high` are two opposite corners (x, y, z) in Blender space. `skip` lists the
    sides to leave out, named after the direction they face: "-x", "+x", "-y", "+y", "-z",
    "+z". A side that is always hidden (it lies on the floor or inside another box) only
    costs triangles, so the caller leaves it out.
    """
    x0, y0, z0 = low
    x1, y1, z1 = high

    # The new corners get the indices first, first + 1, ... first + 7.
    first = len(vertices)
    vertices.extend(
        [
            (x0, y0, z0),  # 0
            (x1, y0, z0),  # 1
            (x0, y1, z0),  # 2
            (x1, y1, z0),  # 3
            (x0, y0, z1),  # 4
            (x1, y0, z1),  # 5
            (x0, y1, z1),  # 6
            (x1, y1, z1),  # 7
        ]
    )

    # Each side lists its 4 corners counter clockwise as seen from outside the box. That
    # order is what makes the normal of the side point outwards.
    sides = {
        "-x": (0, 4, 6, 2),
        "+x": (1, 3, 7, 5),
        "-y": (0, 1, 5, 4),
        "+y": (2, 6, 7, 3),
        "-z": (0, 2, 3, 1),
        "+z": (4, 5, 7, 6),
    }
    for name, corners in sides.items():
        if name not in skip:
            faces.append(tuple(first + corner for corner in corners))


def create_mesh_object(name, vertices, faces):
    """Creates a mesh from the lists, puts it into the scene as an object and returns it.

    The object stays at the origin with no rotation and no scale, so the coordinates in
    the lists are the coordinates of the model.
    """
    mesh = bpy.data.meshes.new(name)
    # The empty list in the middle is for loose edges. The models have none.
    mesh.from_pydata(vertices, [], faces)
    # Flat shading: the exporter writes one normal per face instead of averaging the
    # normals of neighbouring faces at a shared corner. This gives the hard edges.
    mesh.shade_flat()

    model = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(model)
    return model


def box_project_uvs(mesh):
    """Gives every face texture coordinates by projecting it along its dominant axis.

    Every face of the models is perpendicular to one axis. Looking at the face along that
    axis, two coordinates are left, and they become u and v after dividing by
    METRES_PER_UV_UNIT. Because the same division is used everywhere, a texture has the
    same size in metres on every face and is never stretched.

    On the walls v is the height (Blender Z), so the texture stands upright. u follows the
    direction that points to the right for someone looking at the face from outside. That
    is why two opposite sides use opposite signs: without it the texture on one of them
    would be a mirror image.
    """
    uv_layer = mesh.uv_layers.new(name="uv")

    for polygon in mesh.polygons:
        normal = polygon.normal
        # The dominant axis is the one with the largest absolute component of the normal.
        largest = max(abs(normal.x), abs(normal.y), abs(normal.z))

        for loop_index in polygon.loop_indices:
            # A loop is one corner of one face. UVs are stored per loop and not per
            # vertex, because the same vertex needs different UVs on different faces.
            position = mesh.vertices[mesh.loops[loop_index].vertex_index].co

            if abs(normal.x) == largest:
                # Side facing +X or -X: the face lies in the Y-Z plane.
                u = position.y if normal.x > 0.0 else -position.y
                v = position.z
            elif abs(normal.y) == largest:
                # Side facing +Y or -Y: the face lies in the X-Z plane.
                u = -position.x if normal.y > 0.0 else position.x
                v = position.z
            else:
                # Top or bottom: the face lies in the X-Y plane.
                u = position.x
                v = position.y if normal.z > 0.0 else -position.y

            uv_layer.data[loop_index].uv = (u / METRES_PER_UV_UNIT, v / METRES_PER_UV_UNIT)


def assign_textured_material(model, material_name, texture_file):
    """Gives the object one material whose color comes from a PNG in assets/textures."""
    material = bpy.data.materials.new(material_name)
    nodes = material.node_tree.nodes

    # The exporter writes map_Kd only for an Image Texture node that is connected to the
    # Base Color input of the Principled BSDF node, which every new material already has.
    image_node = nodes.new("ShaderNodeTexImage")
    image_node.image = bpy.data.images.load(os.path.join(TEXTURES_DIR, texture_file))
    material.node_tree.links.new(
        image_node.outputs["Color"], nodes["Principled BSDF"].inputs["Base Color"]
    )

    model.data.materials.append(material)


def export_obj(name):
    """Writes everything in the scene to assets/models/<name>.obj and <name>.mtl."""
    os.makedirs(MODELS_DIR, exist_ok=True)
    obj_path = os.path.join(MODELS_DIR, name + ".obj")
    mtl_path = os.path.join(MODELS_DIR, name + ".mtl")

    # Every option that changes the file is set here, also where the value equals the
    # default of this Blender version, so that another version cannot change the output
    # silently.
    bpy.ops.wm.obj_export(
        filepath=obj_path,
        # Blender is Z up, the game is Y up and looks along -Z.
        up_axis="Y",
        forward_axis="NEGATIVE_Z",
        # 1 Blender unit = 1 metre = 1 game unit.
        global_scale=1.0,
        # Position, rotation and scale of the object are baked into the vertex positions,
        # and so are modifiers, so the file needs no extra matrix.
        apply_transform=True,
        apply_modifiers=True,
        export_eval_mode="DAG_EVAL_VIEWPORT",
        # The scene holds only the model at this point, so the whole scene is exported.
        export_selected_objects=False,
        # The loader gets triangles only and does not have to split polygons.
        export_triangulated_mesh=True,
        export_uv=True,
        export_normals=True,
        export_materials=True,
        # Plain MTL only, without the PBR lines (Pr, Pm, ...) the loader does not read.
        export_pbr_extensions=False,
        # The .mtl stores the texture path relative to its own folder, so it is the same
        # on every computer.
        path_mode="RELATIVE",
        # Features the models do not use. Each of them would add other line types.
        export_colors=False,
        export_animation=False,
        export_curves_as_nurbs=False,
        export_object_groups=False,
        export_material_groups=False,
        export_vertex_groups=False,
        export_smooth_groups=False,
    )

    add_diffuse_color(mtl_path)
    print("Wrote", obj_path)
    print("Wrote", mtl_path)


def add_diffuse_color(mtl_path):
    """Inserts a white "Kd" line in front of the "map_Kd" line of the .mtl file.

    Blender leaves Kd (the diffuse color) out when the color comes from a texture. The
    project wants every material to have it: the loader reads Kd as a color that the
    texture is multiplied by, and white leaves the texture unchanged.
    """
    with open(mtl_path, encoding="utf-8", newline="\n") as mtl_file:
        lines = mtl_file.readlines()

    # newline="\n" keeps Unix line endings on Windows too, like the exporter writes them.
    with open(mtl_path, "w", encoding="utf-8", newline="\n") as mtl_file:
        for line in lines:
            if line.startswith("map_Kd "):
                mtl_file.write("Kd 1.000000 1.000000 1.000000\n")
            mtl_file.write(line)


def render_review_shots(name, target, camera_positions):
    """Renders the scene from each camera position, looking at `target`, to PNG files.

    The files go to the temporary folder of the system and never to the repository. They
    are only for looking at a model after changing its script. Call it after export_obj:
    the camera is an object too and would otherwise end up in the .obj file.
    """
    scene = bpy.context.scene
    # Workbench is the simple engine of the Blender viewport. It needs no lights: the
    # "studio" lighting shades the faces by their direction, which shows the shape.
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "TEXTURE"
    # The default view transform (AgX) changes the colors like a film camera would.
    # "Standard" shows the colors of the texture as they are.
    scene.view_settings.view_transform = "Standard"
    scene.render.resolution_x = REVIEW_WIDTH
    scene.render.resolution_y = REVIEW_HEIGHT
    scene.render.image_settings.file_format = "PNG"
    # Workbench fills the background with the color of the world. An empty scene has no
    # world, so one is made: a mid blue-grey on which both light and dark models show.
    scene.world = bpy.data.worlds.new("review_background")
    scene.world.color = (0.30, 0.36, 0.45)

    camera = bpy.data.objects.new("review_camera", bpy.data.cameras.new("review_camera"))
    scene.collection.objects.link(camera)
    scene.camera = camera

    output_dir = os.path.join(tempfile.gettempdir(), "night_maze_review")
    os.makedirs(output_dir, exist_ok=True)

    for number, position in enumerate(camera_positions, start=1):
        camera.location = position
        # A camera looks along its own -Z axis with its +Y axis up. to_track_quat returns
        # the rotation that turns -Z towards the target and keeps Y as upright as possible.
        direction = Vector(target) - Vector(position)
        camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()

        scene.render.filepath = os.path.join(output_dir, f"{name}_{number}.png")
        bpy.ops.render.render(write_still=True)
        print("Review render:", scene.render.filepath)
