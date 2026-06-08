#pragma once

#include "MingEngine/Scene/Physics/Collider3D.hpp"

#include "MingEngine/Engine/Math/FloatRange.hpp"
#include "MingEngine/Engine/Math/Vec2.hpp"

class CylinderZCollider3D : public Collider3D
{
	MCLASS(CylinderZCollider3D, Collider3D);

public:
	CylinderZCollider3D()           = default;
	~CylinderZCollider3D() override = default;

	GameRaycastResult Raycast(RaycastInfo const& info) const override;

protected:
	void Update(float deltaSeconds) override;
	void RenderDebug() const;

public:
	Vec2       m_localStartXY = Vec2::Zero;
	FloatRange m_localZRange  = FloatRange::ZeroToOne;
	float      m_radius       = 0.5f;
};
