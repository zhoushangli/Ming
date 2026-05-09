#include "Engine/Math/FloatRange.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

// Static consts
const FloatRange FloatRange::kZero(0.f, 0.f);
const FloatRange FloatRange::kOne(1.f, 1.f);
const FloatRange FloatRange::kZeroToOne(0.f, 1.f);

// Constructors
FloatRange::FloatRange(float min, float max)
{
    if (min <= max)
    {
        m_min = min;
        m_max = max;
    }
    else
    {
        m_min = max;
        m_max = min;
    }
}

// Operators
FloatRange &FloatRange::operator=(const FloatRange &other)
{
    if (this != &other)
    {
        m_min = other.m_min;
        m_max = other.m_max;
    }
    return *this;
}

bool FloatRange::operator==(const FloatRange &other) const
{
    return m_min == other.m_min && m_max == other.m_max;
}

bool FloatRange::operator!=(const FloatRange &other) const
{
    return !(*this == other);
}

// Methods
bool FloatRange::IsOnRange(float value) const
{
    return value >= m_min && value <= m_max;
}

bool FloatRange::IsOverlap(const FloatRange &other) const
{
    return (m_min < other.m_max) && (other.m_min < m_max);
}

float FloatRange::GetRandomInRange() const
{
    return RandomNumberGenerator::Get().RollRandomFloatInRange(m_min, m_max);
}
