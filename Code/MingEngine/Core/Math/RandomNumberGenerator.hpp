#pragma once

struct Vector2;
struct IntVec2;

class RandomNumberGenerator
{
public:
	static RandomNumberGenerator& Get()
	{
		static RandomNumberGenerator instance;
		return instance;
	}

	int   RollRandomIntLessThan(int maxNotInclusive);
	int   RollRandomIntInRange(int minInclusive, int maxInclusive);
	float RollRandomFloatZeroToOne();
	float RollRandomFloatInRange(float minInclusive, float maxInclusive);

	Vector2 RollRandomVec2InRange(float min, float max);
	Vector2 RollRandomVec2InRange(Vector2 leftBottom, Vector2 rightTop);
	Vector2 RollRandomVec2InRange(float minX, float maxX, float minY, float maxY);
	IntVec2 RollRandomIntVec2InRange(IntVec2 minInclusive, IntVec2 maxInclusive);
};
