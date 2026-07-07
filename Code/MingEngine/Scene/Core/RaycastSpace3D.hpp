#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include <vector>

struct RaycastQuery3D
{
	Vec3       m_start;
	Vec3       m_direction;
	float      m_maxDistance = 10000.f;
	NodeHandle m_exclude     = NodeHandle::Invalid;
};

struct SceneRaycastResult3D : public RaycastResult3D
{
	NodeHandle m_owner;
};

class RaycastObject
{
public:
	// We will first test weather the rough bounds gets hit
	// if so we will then test the actual object for a more accurate hit result
	virtual SceneRaycastResult3D IntersectBounds(RaycastQuery3D const& query) = 0;
	virtual SceneRaycastResult3D IntersectRay(RaycastQuery3D const& query)    = 0;

public:
	NodeHandle m_owner = NodeHandle::Invalid;
};

class RaycastSpace3D
{
public:
	RaycastSpace3D() = default;
	~RaycastSpace3D();

	void AddObject(RaycastObject* object);
	void RemoveObject(RaycastObject* object);

	SceneRaycastResult3D IntersectRay(RaycastQuery3D const& query) const;

private:
	SceneRaycastResult3D UpdateClosestHit(
		SceneRaycastResult3D const& closestHit, SceneRaycastResult3D const& newHit) const;
	bool IntersectWithAABB3(RaycastQuery3D const& query, AABB3 const& bounds) const;

public:
	std::vector<RaycastObject*> m_raycastObjects;
};