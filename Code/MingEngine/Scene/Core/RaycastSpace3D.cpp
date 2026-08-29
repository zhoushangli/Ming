#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

Vector3 RaycastQuery3D::GetStart() const { return m_start; }
Vector3 RaycastQuery3D::GetDirection() const { return m_direction; }
float   RaycastQuery3D::GetMaxDistance() const { return m_maxDistance; }
Node*   RaycastQuery3D::GetExclude() const { return ObjectDatabase::GetInstance<Node>(m_exclude); }

void RaycastQuery3D::SetStart(Vector3 const& start) { m_start = start; }
void RaycastQuery3D::SetDirection(Vector3 const& direction) { m_direction = direction; }
void RaycastQuery3D::SetMaxDistance(float maxDistance) { m_maxDistance = maxDistance; }
void RaycastQuery3D::SetExclude(Node* exclude)
{
	m_exclude = exclude != nullptr ? exclude->GetObjectID() : ObjectID::Invalid;
}

bool    RaycastResult3D::GetDidImpact() const { return m_didImpact; }
float   RaycastResult3D::GetImpactDistance() const { return m_impactDist; }
Vector3 RaycastResult3D::GetImpactPosition() const { return m_impactPos; }
Vector3 RaycastResult3D::GetImpactNormal() const { return m_impactNormal; }
Vector3 RaycastResult3D::GetRayStartPosition() const { return m_rayStartPos; }
Vector3 RaycastResult3D::GetRayForwardNormal() const { return m_rayFwdNormal; }
float   RaycastResult3D::GetRayMaxLength() const { return m_rayMaxLength; }
void RaycastResult3D::SetDidImpact(bool didImpact) { m_didImpact = didImpact; }
void RaycastResult3D::SetImpactDistance(float impactDistance) { m_impactDist = impactDistance; }
void RaycastResult3D::SetImpactPosition(Vector3 const& impactPosition) { m_impactPos = impactPosition; }
void RaycastResult3D::SetImpactNormal(Vector3 const& impactNormal) { m_impactNormal = impactNormal; }
void RaycastResult3D::SetRayStartPosition(Vector3 const& rayStartPosition) { m_rayStartPos = rayStartPosition; }
void RaycastResult3D::SetRayForwardNormal(Vector3 const& rayForwardNormal) { m_rayFwdNormal = rayForwardNormal; }
void RaycastResult3D::SetRayMaxLength(float rayMaxLength) { m_rayMaxLength = rayMaxLength; }

void RaycastSpace3D::BindMethods() {}

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

RaycastResult3D RaycastSpace3D::IntersectRay(RaycastQuery3D const& query) const
{
	RaycastResult3D closestHit;

	for (RaycastObject* object : m_raycastObjects)
	{
		if (object != nullptr && object->m_owner != query.m_exclude)
		{
			RaycastResult3D boundsHit = object->IntersectBounds(query);
			if (!boundsHit.m_didImpact)
			{
				continue;
			}

			RaycastResult3D hit = object->IntersectRay(query);
			if (hit.m_didImpact)
			{
				closestHit = UpdateClosestHit(closestHit, hit);
			}
		}
	}

	return closestHit;
}

RaycastResult3D RaycastSpace3D::UpdateClosestHit(RaycastResult3D const& closestHit, RaycastResult3D const& newHit) const
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
	MathRaycastResult3D result = RaycastVsAABB3D(query.m_start, query.m_direction, query.m_maxDistance, bounds);

	return result.m_didImpact;
}
