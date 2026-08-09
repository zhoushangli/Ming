#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

struct OBB2
{
public:
	OBB2() = default;
	OBB2(Vector2 const& center, Vector2 const& iBasisNormal, Vector2 const& halfDimensions);
	OBB2(Vector2 const& center, Vector2 const& halfDimensions, float orientationDegree);

	void    GetCornerPoints(Vector2* out_fourCornerWorldPositions) const;
	Vector2 GetLocalPosForWorldPos(Vector2 const& worldPos) const;
	Vector2 GetWorldPosForLocalPos(Vector2 const& localPos) const;
	void    RotateAboutCenter(float rotationDeltaDegrees);

public:
	Vector2 m_center;
	Vector2 m_iBasisNormal;
	Vector2 m_halfDimensions;
};
