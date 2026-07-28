#pragma once

#include "MingEngine/Scene/Physics/Collider3D.hpp"

#include "MingEngine/Core/Math/AABB3.hpp"

class AABBCollider3D : public Collider3D
{
	MCLASS(AABBCollider3D, Collider3D);

public:
	AABBCollider3D()           = default;
	~AABBCollider3D() override = default;
	AABBCollider3D(AABB3 const& localBounds);

protected:
	void RenderDebug() const;

public:
	AABB3 m_localBounds;
};
