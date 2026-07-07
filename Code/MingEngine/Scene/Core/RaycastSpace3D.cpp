#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"

RaycastSpace3D::~RaycastSpace3D()
{
	GUARANTEE_OR_DIE(m_raycastObjects.empty(), "RaycastSpace3D destroyed with non-empty object list");

	m_raycastObjects.clear();
}

void RaycastSpace3D::AddObject(RaycastObject* object)
{
	if (object != nullptr)
	{
		m_raycastObjects.push_back(object);
	}
}

void RaycastSpace3D::RemoveObject(RaycastObject* object)
{
	if (object != nullptr)
	{
		auto iter = std::find(m_raycastObjects.begin(), m_raycastObjects.end(), object);
		if (iter != m_raycastObjects.end())
		{
			m_raycastObjects.erase(iter);
		}
	}
}

SceneRaycastResult3D RaycastSpace3D::IntersectRay(RaycastQuery3D const& query) const
{
	SceneRaycastResult3D closestHit;

	for (RaycastObject* object : m_raycastObjects)
	{
		if (object != nullptr && object->m_owner != query.m_exclude)
		{
			SceneRaycastResult3D boundsHit = object->IntersectBounds(query);
			if (!boundsHit.m_didImpact)
			{
				continue;
			}

			SceneRaycastResult3D hit = object->IntersectRay(query);
			if (hit.m_didImpact)
			{
				closestHit = UpdateClosestHit(closestHit, hit);
			}
		}
	}

	return closestHit;
}

SceneRaycastResult3D RaycastSpace3D::UpdateClosestHit(
	SceneRaycastResult3D const& closestHit, SceneRaycastResult3D const& newHit) const
{
	if (!closestHit.m_didImpact)
	{
		return newHit;
	}
	else if (newHit.m_didImpact && newHit.m_impactDist < closestHit.m_impactDist)
	{
		return newHit;
	}

	return closestHit;
}

bool RaycastSpace3D::IntersectWithAABB3(RaycastQuery3D const& query, AABB3 const& bounds) const
{
	RaycastResult3D result = RaycastVsAABB3D(query.m_start, query.m_direction, query.m_maxDistance, bounds);

	return result.m_didImpact;
}
