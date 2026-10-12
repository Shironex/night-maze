// PeriodicAverage: the average of a number that is measured in every frame.
#pragma once

namespace core {

/// Collects measurements of one number and shows their average, a new one at intervals.
///
/// A time that is measured in every frame jumps from frame to frame, so a number that
/// is drawn again 60 times a second cannot be read. This class is the same answer the
/// FPS display has (core::Time): add() collects, and publish() makes the average of
/// what was collected the value that is shown until the next publish(). Who owns the
/// object decides when that is: this class asks no clock.
class PeriodicAverage {
public:
    /// Adds one measurement to the ones collected since the last publish().
    void add(double value);

    /// Ends the interval: value() becomes the average of the measurements added since
    /// the last call, and the collecting starts again. Without any measurement there is
    /// no average: hasValue() becomes false.
    void publish();

    /// True when the last interval had at least one measurement.
    bool hasValue() const { return m_hasValue; }

    /// The average of the last interval, or 0 while hasValue() is false.
    double value() const { return m_value; }

private:
    // What add() has collected since the last publish().
    double m_sum = 0.0;
    int m_count = 0;

    // What the last publish() computed.
    double m_value = 0.0;
    bool m_hasValue = false;
};

} // namespace core
