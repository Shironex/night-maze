// Tests of core::PeriodicAverage: the average of what was measured in one interval.
#include "core/PeriodicAverage.hpp"

#include <doctest/doctest.h>

TEST_CASE("a new average has no value") {
    const core::PeriodicAverage average;
    CHECK_FALSE(average.hasValue());
    CHECK(average.value() == doctest::Approx(0.0));
}

TEST_CASE("publish makes the average of the added measurements the value") {
    core::PeriodicAverage average;
    average.add(2.0);
    average.add(4.0);
    average.add(9.0);
    // Nothing is shown before the interval has ended.
    CHECK_FALSE(average.hasValue());

    average.publish();
    CHECK(average.hasValue());
    CHECK(average.value() == doctest::Approx(5.0));
}

TEST_CASE("the value stays while the next interval is collected") {
    core::PeriodicAverage average;
    average.add(10.0);
    average.publish();

    average.add(30.0);
    CHECK(average.value() == doctest::Approx(10.0));

    // The second interval starts from nothing: the 10 is not part of its average.
    average.publish();
    CHECK(average.value() == doctest::Approx(30.0));
}

TEST_CASE("an interval without a measurement has no value") {
    core::PeriodicAverage average;
    average.add(7.0);
    average.publish();
    REQUIRE(average.hasValue());

    average.publish();
    CHECK_FALSE(average.hasValue());
    CHECK(average.value() == doctest::Approx(0.0));
}
