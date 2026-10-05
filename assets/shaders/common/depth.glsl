// Depth shared by the post-processing shaders: from the number stored in a depth
// texture back to a distance in metres. This file is not a shader of its own. It has no
// #version line: the shader loader puts its text in place of the #include line.
// See docs/modules/renderer/post-process.md

// The distance from the camera plane to a surface, in metres, from the value a depth
// texture stores for it. near and far are the clipping planes of the projection the
// scene was drawn with (scene::Camera).
//
// A perspective projection does not store the distance itself. It stores a value from
// 0 (near plane) to 1 (far plane) that changes fast close to the camera and hardly at
// all far away: with planes at 0.1 m and 100 m, a wall 2 m away already has 0.95. This
// function undoes that. It is the projection formula solved for the distance:
//   ndc = 2 * stored - 1                      (back from 0..1 to -1..1)
//   distance = 2 * near * far / (far + near - ndc * (far - near))
float linearDepth(float stored, float near, float far) {
    float ndc = 2.0 * stored - 1.0;
    return 2.0 * near * far / (far + near - ndc * (far - near));
}
