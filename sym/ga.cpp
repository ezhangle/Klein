#include "ga.hpp"

#include <cassert>
#include <immintrin.h>

uint32_t popcnt(uint32_t i)
{
    return _mm_popcnt_u32(i);
}

algebra::algebra(uint32_t p, uint32_t q, uint32_t r)
    : p_{p}
    , q_{q}
    , r_{r}
    , dim_{p + q + r}
{
    assert(dim_ < 32
           && "Exceeded maximum metric size that can fit in a 32-bit integer");
}

bool algebra::operator==(algebra const& other) const noexcept
{
    return p_ == other.p_ && q_ == other.q_ && r_ == other.r_;
}

int32_t algebra::mul(uint32_t lhs, uint32_t rhs) const noexcept
{
    if (lhs == 0 && rhs == 0)
    {
        return 1;
    }
    else if (lhs == 0)
    {
        return rhs + 1;
    }
    else if (rhs == 0)
    {
        return lhs + 1;
    }

    int factor     = 1;
    int32_t result = 0;

    for (uint32_t i = 0; i != dim_; ++i)
    {
        uint8_t index = dim_ - i - 1;
        uint8_t bit   = 1 << index;
        bool lhs_e    = (lhs & bit) > 0;
        bool rhs_e    = (rhs & bit) > 0;

        if (lhs_e)
        {
            if ((popcnt(rhs & (bit - 1)) & 1) > 0)
            {
                factor = -factor;
            }

            if (rhs_e)
            {
                // Metric contraction
                if (r_ > 0 && index < r_)
                {
                    // Degenerate
                    return 0;
                }
                else if (index >= p_ + r_)
                {
                    factor = -factor;
                }
            }
        }
    }

    return factor * ((lhs ^ rhs) + 1);
}

bool algebra::rev(uint32_t in) const noexcept
{
    uint32_t bits = popcnt(in);
    return (bits & 0b10) > 0;
}

mv::mv(algebra const& a) noexcept
    : algebra_{&a}
{}

mv& mv::push(uint32_t e, poly const& p) noexcept
{
    auto it = terms.find(e);
    if (it == terms.end())
    {
        terms.emplace(e, p);
    }
    else
    {
        it->second += p;
    }

    return *this;
}

mv mv::operator-() const& noexcept
{
    mv out{*this};
    for (auto& [e, p] : out.terms)
    {
        p = -p;
    }
    return out;
}

mv& mv::operator-() && noexcept
{
    for (auto& [e, p] : terms)
    {
        if (algebra_->rev(e))
        {
            p = -p;
        }
    }
    return *this;
}

mv mv::operator~() const& noexcept
{
    mv out{*this};
    for (auto& [e, p] : out.terms)
    {
        if (algebra_->rev(e))
        {
            p = -p;
        }
    }
    return out;
}

mv& mv::operator~() && noexcept
{
    for (auto& [e, p] : terms)
    {
        p = -p;
    }
    return *this;
}

mv& mv::operator+=(mv const& other) noexcept
{
    for (auto&& [e, p] : other.terms)
    {
        push(e, p);
    }

    return *this;
}

mv& mv::operator-=(mv const& other) noexcept
{
    for (auto&& [e, p] : other.terms)
    {
        push(e, -p);
    }

    return *this;
}

mv operator+(mv const& lhs, mv const& rhs) noexcept
{
    mv out{lhs};
    out += rhs;
    return out;
}

mv& mv::operator*=(mv const& other) noexcept
{
    mv result = *this * other;
    std::swap(terms, result.terms);
    return *this;
}

mv operator*(mv const& lhs, mv const& rhs) noexcept
{
    mv out{*lhs.algebra_};

    for (auto&& [e1, p1] : lhs.terms)
    {
        for (auto&& [e2, p2] : rhs.terms)
        {
            int32_t result = lhs.algebra_->mul(e1, e2);
            if (result != 0)
            {
                uint32_t e = std::abs(result) - 1;
                auto it    = out.terms.find(e);

                if (it == out.terms.end())
                {
                    it = out.terms.emplace(e, poly{}).first;
                }

                if (result < 0)
                {
                    it->second += -(p1 * p2);
                }
                else
                {
                    it->second += (p1 * p2);
                }
            }
        }
    }

    // Remove empty terms
    for (auto it = out.terms.begin(); it != out.terms.end();)
    {
        if (it->second.terms.empty())
        {
            it = out.terms.erase(it);
        }
        else
        {
            ++it;
        }
    }

    return out;
}

std::ostream&
operator<<(std::ostream& os,
           std::map<std::string, int>::const_iterator const& it) noexcept
{
    if (it->second == 0)
    {
        return os;
    }

    os << it->first;
    if (it->second != 1)
    {
        os << '^' << it->second;
    }
    return os;
}

std::ostream&
operator<<(std::ostream& os,
           std::unordered_map<mon, float>::const_iterator const& it) noexcept
{
    if (it->second == -1.f)
    {
        os << '-';
    }
    else if (it->second != 1.f)
    {
        os << it->second;
    }

    if (it->first.factors.empty())
    {
        return os;
    }

    auto factor = it->first.factors.cbegin();
    os << factor;
    ++factor;

    for (; factor != it->first.factors.cend(); ++factor)
    {
        os << ' ' << factor;
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, poly const& p) noexcept
{
    if (p.terms.empty())
    {
        return os;
    }

    bool multi = p.terms.size() > 1;

    if (multi)
    {
        os << '(';
    }

    auto it = p.terms.cbegin();
    os << it;
    ++it;

    for (; it != p.terms.cend(); ++it)
    {
        os << " + " << it;
    }

    if (multi)
    {
        os << ')';
    }

    return os;
}

std::ostream&
operator<<(std::ostream& os,
           std::unordered_map<uint32_t, poly>::const_iterator const& it) noexcept
{
    os << it->second;

    if (it->first == 0)
    {
        return os;
    }

    os << " e";

    for (size_t i = 0; i != 9; ++i)
    {
        if ((it->first & (1 << i)) > 0)
        {
            os << i;
        }
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, mv const& m) noexcept
{
    if (m.terms.empty())
    {
        os << '1';
        return os;
    }

    auto it = m.terms.cbegin();
    os << it;
    ++it;

    for (; it != m.terms.cend(); ++it)
    {
        os << " + " << it;
    }

    return os;
}