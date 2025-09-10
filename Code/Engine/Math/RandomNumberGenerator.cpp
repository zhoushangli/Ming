#include "RandomNumberGenerator.hpp"
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

