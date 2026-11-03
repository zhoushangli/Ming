#pragma once

class IntRange
{
public:
	// Constructors
	IntRange() = default;
	explicit IntRange(int min, int max);

	// Operators
	IntRange& operator=(const IntRange& other);
	bool      operator==(const IntRange& other) const;
	bool      operator!=(const IntRange& other) const;

	// Methods
	bool IsOnRange(int value) const;
	bool IsOverlappingWith(const IntRange& other) const;

public:
	int m_min = 0;
	int m_max = 0;

	// Named static consts
	static const IntRange Zero;
	static const IntRange One;
	static const IntRange ZeroToOne;
};
