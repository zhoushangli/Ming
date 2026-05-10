#include "Engine/Math/IntRange.hpp"

// Static consts
const IntRange IntRange::Zero(0, 0);
const IntRange IntRange::One(1, 1);
const IntRange IntRange::ZeroToOne(0, 1);

// Constructors
IntRange::IntRange(int min, int max) : m_min(min), m_max(max) {}

// Operators
IntRange& IntRange::operator=(const IntRange& other)
{
	if (this != &other)
	{
		m_min = other.m_min;
		m_max = other.m_max;
	}
	return *this;
}

bool IntRange::operator==(const IntRange& other) const { return m_min == other.m_min && m_max == other.m_max; }

bool IntRange::operator!=(const IntRange& other) const { return !(*this == other); }

// Methods
bool IntRange::IsOnRange(int value) const { return value >= m_min && value <= m_max; }

bool IntRange::IsOverlappingWith(const IntRange& other) const { return !(m_max < other.m_min || m_min > other.m_max); }
