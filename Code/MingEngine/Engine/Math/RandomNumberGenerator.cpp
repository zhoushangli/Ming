#include "MingEngine/Engine/Math/RandomNumberGenerator.hpp"

#include "MingEngine/Engine/Math/Vec2.hpp"
#include "MingEngine/Engine/Math/IntVec2.hpp"

#include <cstdlib>
#include <ctime>

int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive)
{
	if (maxNotInclusive <= 0) 
    {
        return 0;
    }

	return std::rand() % maxNotInclusive;
}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive)
{
	if (minInclusive >= maxInclusive) 
	{
		return minInclusive;
	}

	int range = maxInclusive - minInclusive + 1;
	return minInclusive + std::rand() % range;
}

float RandomNumberGenerator::RollRandomFloatZeroToOne()
{
	return (float)((float)std::rand() / float(RAND_MAX));
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive)
{
	float t = RollRandomFloatZeroToOne();
	return minInclusive + t * (maxInclusive - minInclusive);
}

Vec2 RandomNumberGenerator::RollRandomVec2InRange(float min, float max)
{
	Vec2 output;
	output.x = RollRandomFloatInRange(min, max);
	output.y = RollRandomFloatInRange(min, max);
	return output;
}

Vec2 RandomNumberGenerator::RollRandomVec2InRange(Vec2 leftBottom, Vec2 rightTop)
{
	Vec2 output;
	output.x = RollRandomFloatInRange(leftBottom.x, rightTop.x);
	output.y = RollRandomFloatInRange(leftBottom.y, rightTop.y);
	return output;
}

Vec2 RandomNumberGenerator::RollRandomVec2InRange(float minX, float maxX, float minY, float maxY)
{
	Vec2 output;
	output.x = RollRandomFloatInRange(minX, maxX);
	output.y = RollRandomFloatInRange(minY, maxY);
	return output;
}

IntVec2 RandomNumberGenerator::RollRandomIntVec2InRange(IntVec2 minInclusive, IntVec2 maxInclusive)
{
    IntVec2 output;
    output.x = RollRandomIntInRange(minInclusive.x, maxInclusive.x);
    output.y = RollRandomIntInRange(minInclusive.y, maxInclusive.y);
    return output;
}

