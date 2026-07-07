#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
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

struct RaycastHit3D
{
	bool       m_didHit   = false;
	float      m_distance = 0.f;
	Vec3       m_position;
	Vec3       m_normal;
	NodeHandle m_object;
	NodeHandle m_owner;
};

class RaycastObject
{
public:
	virtual RaycastHit3D IntersectRay(RaycastQuery3D const& query) = 0;

public:
	NodeHandle m_handle = NodeHandle::Invalid;
	NodeHandle m_owner  = NodeHandle::Invalid;
	AABB3      m_worldBounds;
};

class RaycastSpace3D
{
public:
	RaycastSpace3D() = default;
	~RaycastSpace3D();

	void AddObject(RaycastObject* object);
	void RemoveObject(RaycastObject* object);

	RaycastHit3D IntersectRay(RaycastQuery3D const& query) const;

public:
	std::vector<RaycastObject*> m_raycastObjects;
};