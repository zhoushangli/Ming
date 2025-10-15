#pragma once

#include "Engine/Math/Vec2.hpp"

class RandomNumberGenerator
{
public:
    static RandomNumberGenerator& Get()
    {
        static RandomNumberGenerator instance;
        return instance;
    }
    
    int RollRandomIntLessThan(int maxNotInclusive);
    int RollRandomIntInRange(int minInclusive, int maxInclusive);
    float RollRandomFloatZeroToOne();
    float RollRandomFloatInRange(float minInclusive, float maxInclusive);

    Vec2 RollRandomVec2InRange(float min, float max);
    Vec2 RollRandomVec2InRange(Vec2 leftBottom, Vec2 rightTop);
    Vec2 RollRandomVec2InRange(float minX, float maxX, float minY, float maxY);
};
