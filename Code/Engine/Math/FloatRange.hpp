#pragma once

class FloatRange
{
public:
    float m_min = 0.f;
    float m_max = 0.f;

    // Constructors
    FloatRange() = default;
    explicit FloatRange(float min, float max);

    // Operators
    FloatRange& operator=(const FloatRange& other);
    bool operator==(const FloatRange& other) const;
    bool operator!=(const FloatRange& other) const;

    // Methods
    bool IsOnRange(float value) const;
    bool IsOverlappingWith(const FloatRange& other) const;

    // Named static consts
    static const FloatRange ZERO;
    static const FloatRange ONE;
    static const FloatRange ZERO_TO_ONE;
};

