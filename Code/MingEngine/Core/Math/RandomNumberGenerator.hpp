#pragma once


struct Vec2;
struct IntVec2;

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
    IntVec2 RollRandomIntVec2InRange(IntVec2 minInclusive, IntVec2 maxInclusive);
};
