// ComparisonSampler: a sampler object that makes a depth texture answer "lit or in shadow".
// See docs/modules/renderer/shadows.md
#include "gfx/ComparisonSampler.hpp"

#include "core/GlCheck.hpp"

#include <array>

namespace gfx {

namespace {

// The depth outside the texture: 1 is the far plane of the light. OpenGL takes a border
// as a colour of four numbers and uses the first one for a depth texture.
constexpr std::array<GLfloat, 4> FAR_PLANE_BORDER = {1.0F, 1.0F, 1.0F, 1.0F};

} // namespace

ComparisonSampler::ComparisonSampler() {
    GL_CHECK(glGenSamplers(1, &m_id));

    // A sampler object is changed through its id: nothing has to be bound first.
    //
    // The comparison: the third coordinate of the lookup is compared with the stored
    // depth, and the result is 1 where it is less than or equal to it.
    GL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE));
    GL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL));

    // Outside the texture the border is read, in both directions.
    GL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    GL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    GL_CHECK(glSamplerParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, FAR_PLANE_BORDER.data()));

    setLinearFilter(m_linear);
}

ComparisonSampler::~ComparisonSampler() {
    // OpenGL silently ignores the id 0.
    GL_CHECK(glDeleteSamplers(1, &m_id));
}

void ComparisonSampler::bind(GLuint unit) const {
    // glBindSampler takes the plain number of the unit, not GL_TEXTURE0 + unit.
    GL_CHECK(glBindSampler(unit, m_id));
}

void ComparisonSampler::setLinearFilter(bool linear) {
    m_linear = linear;
    // A shadow map has one level, so the minification filter is one without mipmaps.
    const GLint filter = linear ? GL_LINEAR : GL_NEAREST;
    GL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_MIN_FILTER, filter));
    GL_CHECK(glSamplerParameteri(m_id, GL_TEXTURE_MAG_FILTER, filter));
}

} // namespace gfx
