// PeriodicAverage: the average of a number that is measured in every frame.
#include "core/PeriodicAverage.hpp"

namespace core {

void PeriodicAverage::add(double value) {
    m_sum += value;
    m_count += 1;
}

void PeriodicAverage::publish() {
    m_hasValue = m_count > 0;
    // Without a measurement the division would be 0 / 0.
    m_value = m_hasValue ? m_sum / m_count : 0.0;
    m_sum = 0.0;
    m_count = 0;
}

} // namespace core
