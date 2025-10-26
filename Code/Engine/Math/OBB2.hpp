#pragma once

class IntRange
{
public:
    int m_min = 0;
    int m_max = 0;

    // Constructors
    IntRange() = default;
    explicit IntRange(int min, int max);

    // Operators
    IntRange& operator=(const IntRange& other);
    bool operator==(const IntRange& other) const;
    bool operator!=(const IntRange& other) const;

    // Methods
    bool IsOnRange(int value) const;
    bool IsOverlappingWith(const IntRange& other) const;

    // Named static consts
    static const IntRange ZERO;
    static const IntRange ONE;
    static const IntRange ZERO_TO_ONE;
};

