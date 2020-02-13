#include <doctest/doctest.h>

#include <klein/klein.hpp>

using namespace kln;

TEST_CASE("simd-sandwich")
{
    __m128 a = _mm_set_ps(4.f, 3.f, 2.f, 1.f);
    __m128 b = _mm_set_ps(-1.f, -2.f, -3.f, -4.f);
    float ab[4];
    _mm_store_ps(ab, detail::sw02(a, b));

    CHECK_EQ(ab[0], 1.f);
    CHECK_EQ(ab[1], 2.f);
    CHECK_EQ(ab[2], 3.f);
    CHECK_EQ(ab[3], -24.f);
}

TEST_CASE("rotor-point")
{
    rotor r{M_PI * 0.5f, 0, 0, 1.f};
    point p1{1, 0, 0};
    point p2 = r(p1);
    CHECK_EQ(p2.x(), 0.f);
    CHECK_EQ(p2.y(), doctest::Approx(1.f));
    CHECK_EQ(p2.z(), 0.f);
}

TEST_CASE("translator-point")
{
    translator t{1.f, 0.f, 0.f, 1.f};
    point p1{1, 0, 0};
    point p2 = t(p1);
    CHECK_EQ(p2.x(), 1.f);
    CHECK_EQ(p2.y(), 0.f);
    CHECK_EQ(p2.z(), 1.f);
}

TEST_CASE("construct-motor")
{
    rotor r{M_PI * 0.5f, 0, 0, 1.f};
    translator t{1.f, 0.f, 0.f, 1.f};
    motor m = r * t;
    point p1{1, 0, 0};
    point p2 = m(p1);
    CHECK_EQ(p2.x(), 0.f);
    CHECK_EQ(p2.y(), doctest::Approx(1.f));
    CHECK_EQ(p2.z(), doctest::Approx(1.f));
}

TEST_CASE("motor-plane")
{
    motor m{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
    plane p1{3.f, 2.f, 1.f, -1.f};
    plane p2 = m(p1);
    CHECK_EQ(p2.x(), 78.f);
    CHECK_EQ(p2.y(), 60.f);
    CHECK_EQ(p2.z(), 54.f);
    CHECK_EQ(p2.d(), 358.f);
}

TEST_CASE("motor-point")
{
    motor m{1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
    point p1{-1.f, 1.f, 2.f};
    point p2 = m(p1);
    CHECK_EQ(p2.x(), -12.f);
    CHECK_EQ(p2.y(), -86.f);
    CHECK_EQ(p2.z(), -86.f);
    CHECK_EQ(p2.w(), 30.f);
}