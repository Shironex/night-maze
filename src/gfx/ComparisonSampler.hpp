// ComparisonSampler: a sampler object that makes a depth texture answer "lit or in shadow".
// See docs/modules/renderer/shadows.md
#pragma once

#include <glad/gl.h>

namespace gfx {

/// Owns one OpenGL sampler object set up for reading a shadow map.
///
/// A sampler object holds the rules a texture is read with (filter, wrapping), apart
/// from the texture itself. While one is bound to a texture unit, its rules replace the
/// ones stored in the texture bound to that unit. This one has three rules a plain
/// depth texture does not have:
///
///   - COMPARISON. The shader does not get the stored depth. It hands in a depth of its
///     own, and the graphics card answers 1 when that depth is nearer to the light than
///     the stored one or equal to it (GL_LEQUAL: lit) and 0 when it is farther away (in
///     shadow). In GLSL such a texture is read through a sampler2DShadow.
///   - LINEAR FILTER. With a comparison the filter does not blend depths: the card
///     compares the four texels around the place and blends the four ANSWERS. That is
///     percentage closer filtering done by the hardware, 2 x 2 texels, for the price of
///     one lookup. setLinearFilter(false) switches it off.
///   - A BORDER. Outside the texture (coordinates below 0 or above 1) the stored depth
///     counts as 1, the far plane. Nothing is farther away than that, so everything
///     outside the shadow map is lit.
///
/// The rules live in a sampler object and not in the depth texture on purpose: the
/// texture is also read as a plain picture, without any sampler object, when its
/// preview is drawn. With the comparison set on the texture itself that read would be
/// undefined.
///
/// The constructor creates the object, the destructor deletes it (RAII). It cannot be
/// copied: a copy would hold the same id and delete it a second time. It needs a current
/// OpenGL context for its whole life, so it must be destroyed before the window.
class ComparisonSampler {
public:
    /// Creates the sampler object with the three rules above.
    ComparisonSampler();
    ~ComparisonSampler();

    ComparisonSampler(const ComparisonSampler&) = delete;
    ComparisonSampler& operator=(const ComparisonSampler&) = delete;

    /// Binds the sampler object to texture unit number unit. The texture bound to that
    /// unit is read with its rules from now on, until another sampler object (or none)
    /// is bound there. The active texture unit is not changed.
    void bind(GLuint unit) const;

    /// Chooses between the linear filter (true: four comparisons, blended) and the
    /// nearest filter (false: one comparison). It can be called at any time, bound or
    /// not.
    void setLinearFilter(bool linear);

    /// The filter in use: true for linear.
    bool linearFilter() const { return m_linear; }

private:
    // Name (id) of the OpenGL sampler object.
    GLuint m_id = 0;
    bool m_linear = true;
};

} // namespace gfx
