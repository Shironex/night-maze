# Tangents are computed once when an OBJ loads, and the shader rebuilds the bitangent
Status: accepted (2026-10-05)
Code: src/assets/Tangents.cpp, src/assets/ObjLoader.cpp, assets/shaders/common/normal_map.glsl

Context: Normal maps need a tangent per vertex, and OBJ files have none. The models are flat shaded Blender exports.
Decision: assets::computeTangents runs at the end of parseObj from positions and UVs, makes the tangent perpendicular to the normal once, and stores three floats per vertex. The shader uses cross(N, T) with no handedness sign. countMirroredTriangles warns when a model breaks that.
Why: I rejected tangents in the file because it needs another format or a second file whose vertex order must match. Gram-Schmidt in the vertex shader was rejected as repeated every frame and untestable.
Cost and revisit: Mirrored UVs would shade wrongly. I reopen it if a model has them.
