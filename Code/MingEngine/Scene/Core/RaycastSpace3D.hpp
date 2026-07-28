#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

#include <vector>

class Node;

class RaycastQuery3D : public RefCounted
{
	MCLASS(RaycastQuery3D, RefCounted);

public:
	RaycastQuery3D() = default;
	RaycastQuery3D(MathRaycastQuery3D const& other) { this->operator=(other); }

	Vec3  GetStart() const;
	Vec3  GetDirection() const;
	float GetMaxDistance() const;
	Node* GetExclude() const;

	void SetStart(Vec3 const& start);
	void SetDirection(Vec3 const& direction);
	void SetMaxDistance(float maxDistance);
	void SetExclude(Node* exclude);

	RaycastQuery3D& operator=(MathRaycastQuery3D const& other)
	{
		m_start       = other.m_startPos;
		m_direction   = other.m_forwardNormal;
		m_maxDistance = other.m_maxLength;

		return *this;
	}

protected:
	static void BindMethods();

public:
	Vec3       m_start;
	Vec3       m_direction;
	float      m_maxDistance = 10000.f;
	NodeHandle m_exclude     = NodeHandle::Invalid;

private:
	Node* m_excludeNode = nullptr;
};

class RaycastResult3D : public RefCounted
{
	MCLASS(RaycastResult3D, RefCounted);

public:
	RaycastResult3D() = default;
	RaycastResult3D(MathRaycastResult3D const& other) { this->operator=(other); }

	RaycastResult3D& operator=(MathRaycastResult3D const& other)
	{
		m_didImpact    = other.m_didImpact;
		m_impactDist   = other.m_impactDist;
		m_impactPos    = other.m_impactPos;
		m_impactNormal = other.m_impactNormal;
		m_rayStartPos  = other.m_rayStartPos;
		m_rayFwdNormal = other.m_rayFwdNormal;
		m_rayMaxLength = other.m_rayMaxLength;

		return *this;
	}

	bool  GetDidImpact() const;
	float GetImpactDistance() const;
	Vec3  GetImpactPosition() const;
	Vec3  GetImpactNormal() const;
	Vec3  GetRayStartPosition() const;
	Vec3  GetRayForwardNormal() const;
	float GetRayMaxLength() const;
	int   GetOwner() const;

	void SetDidImpact(bool didImpact);
	void SetImpactDistance(float impactDistance);
	void SetImpactPosition(Vec3 const& impactPosition);
	void SetImpactNormal(Vec3 const& impactNormal);
	void SetRayStartPosition(Vec3 const& rayStartPosition);
	void SetRayForwardNormal(Vec3 const& rayForwardNormal);
	void SetRayMaxLength(float rayMaxLength);

protected:
	static void BindMethods();

public:
	// Basic raycast result information (required)
	bool  m_didImpact  = false;
	float m_impactDist = 0.f;
	Vec3  m_impactPos;
	Vec3  m_impactNormal;

	// Original raycast information (optional)
	Vec3  m_rayStartPos;
	Vec3  m_rayFwdNormal;
	float m_rayMaxLength = 1.f;

	NodeHandle m_owner;
};

class RaycastObject
{
public:
	// We will first test weather the rough bounds gets hit
	// if so we will then test the actual object for a more accurate hit result
	virtual RaycastResult3D IntersectBounds(RaycastQuery3D const& query) = 0;
	virtual RaycastResult3D IntersectRay(RaycastQuery3D const& query)    = 0;

public:
	NodeHandle m_owner = NodeHandle::Invalid;
};

class RaycastSpace3D : public Object
{
	MCLASS(RaycastSpace3D, Object);

public:
	RaycastSpace3D() = default;
	~RaycastSpace3D();

	void AddObject(RaycastObject* object);
	void RemoveObject(RaycastObject* object);

	RaycastResult3D  IntersectRay(RaycastQuery3D const& query) const;
	RaycastResult3D* IntersectRayScript(RaycastQuery3D* query) const;

	static void BindMethods();

private:
	RaycastResult3D UpdateClosestHit(RaycastResult3D const& closestHit, RaycastResult3D const& newHit) const;
	bool            IntersectWithAABB3(RaycastQuery3D const& query, AABB3 const& bounds) const;

public:
	std::vector<RaycastObject*> m_raycastObjects;
};
