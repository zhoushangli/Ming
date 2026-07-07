#pragma once

#include "MingEngine/Scene/Physics/Collider3D.hpp"

#include "MingEngine/Core/Math/FloatRange.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"

class CylinderZCollider3D : public Collider3D
{
	MCLASS(CylinderZCollider3D, Collider3D);

public:
	CylinderZCollider3D()           = default;
	~CylinderZCollider3D() override = default;

	GameRaycastResult Raycast(RaycastInfo const& info) const override;

protected:
	void RenderDebug() const;

public:
	Vec2       m_localStartXY = Vec2::Zero;
	FloatRange m_localZRange  = FloatRange::ZeroToOne;
	float      m_radius       = 0.5f;
};
