#pragma once

#include "MingEngine/Scene/Physics/Collider3D.hpp"

#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

class CapsuleCollider3D : public Collider3D
{
	MCLASS(CapsuleCollider3D, Collider3D);

public:
	CapsuleCollider3D()           = default;
	~CapsuleCollider3D() override = default;
	CapsuleCollider3D(Capsule3 const& capsule);

protected:
	void RenderDebug() const;

public:
	Vector3 m_localStart = Vector3::Zero;
	Vector3 m_localEnd   = Vector3::Zero;
	float   m_radius     = 0.5f;
};
