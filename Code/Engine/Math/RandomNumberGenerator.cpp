#include "RandomNumberGenerator.hpp"
#include <random>

int RandomNumberGenerator::RollRandomIntLessThan(int maxNotInclusive)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, maxNotInclusive - 1);
    return dist(gen);
}

int RandomNumberGenerator::RollRandomIntInRange(int minInclusive, int maxInclusive)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(minInclusive, maxInclusive);
    return dist(gen);
}

float RandomNumberGenerator::RollRandomFloatZeroToOne()
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(gen);
}

float RandomNumberGenerator::RollRandomFloatInRange(float minInclusive, float maxInclusive)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(minInclusive, maxInclusive);
    return dist(gen);
}

