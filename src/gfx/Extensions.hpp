// Extensions: asks the graphics driver whether it offers an OpenGL extension.
// See docs/modules/gfx/textures.md
#pragma once

namespace gfx {

/// True when the driver lists the extension called name (for example
/// "GL_EXT_texture_filter_anisotropic"). An extension is a feature that is not part of
/// OpenGL 4.1 Core, so its constants may be used only after this returned true.
/// It needs a current OpenGL context.
bool hasExtension(const char* name);

} // namespace gfx
