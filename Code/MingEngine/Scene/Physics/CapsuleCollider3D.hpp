#pragma once

#include "MingEngine/Scene/Physics/Collider3D.hpp"

#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

class CapsuleCollider3D : public Collider3D
{
	MCLASS(CapsuleCollider3D, Collider3D);

public:
	CapsuleCollider3D()           = default;
	~CapsuleCollider3D() override = default;
	CapsuleCollider3D(Capsule3 const& capsule);

	GameRaycastResult Raycast(RaycastInfo const& info) const override;

protected:
	void RenderDebug() const;

public:
	Vec3  m_localStart = Vec3::Zero;
	Vec3  m_localEnd   = Vec3::Zero;
	float m_radius     = 0.5f;
};
