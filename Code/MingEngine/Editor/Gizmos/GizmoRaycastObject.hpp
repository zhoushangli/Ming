#pragma once

#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

class GizmoComponent;

enum class GizmoAxis;

// Base shape for gizmo raycast geometry.
// All positions are in world space, computed fresh each frame from gizmo context.
class GizmoShape
{
public:
	virtual ~GizmoShape() = default;

	// Returns the world-space bounding box.
	virtual AABB3 GetWorldAABB() const = 0;

	// Tests ray intersection in world space. Returns hit distance and position.
	virtual bool IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const = 0;
};

// --- Arrow shaft (cylinder segment) ---
class GizmoCylinderShape : public GizmoShape
{
public:
	GizmoCylinderShape(Vec3 const& worldStart, Vec3 const& worldEnd, float radius);

	AABB3 GetWorldAABB() const override;
	bool  IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const override;

public:
	Vec3  m_worldStart;
	Vec3  m_worldEnd;
	float m_radius;
};

// --- Arrow tip / cone ---
class GizmoConeShape : public GizmoShape
{
public:
	GizmoConeShape(Vec3 const& worldStart, Vec3 const& worldEnd, float radius);

	AABB3 GetWorldAABB() const override;
	bool  IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const override;

public:
	Vec3  m_worldStart;
	Vec3  m_worldEnd;
	float m_radius;
};

// --- Plane square ---
class GizmoQuadShape : public GizmoShape
{
public:
	GizmoQuadShape(
		Vec3 const& worldCenter, Vec3 const& worldU, Vec3 const& worldV, float halfExtent, Vec3 const& worldNormal);

	AABB3 GetWorldAABB() const override;
	bool  IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const override;

public:
	Vec3  m_worldCenter;
	Vec3  m_worldU;
	Vec3  m_worldV;
	Vec3  m_worldNormal;
	float m_halfExtent;
};

// --- Torus arc (rotation ring) ---
class GizmoTorusArcShape : public GizmoShape
{
public:
	GizmoTorusArcShape(
		Vec3 const& worldOrigin,
		Vec3 const& worldU,
		Vec3 const& worldV,
		float       radius,
		float       tubeRadius,
		Vec3 const& worldNormal);

	AABB3 GetWorldAABB() const override;
	bool  IntersectRay(RaycastQuery3D const& query, float& outDist, Vec3& outPos) const override;

public:
	Vec3  m_worldOrigin;
	Vec3  m_worldU;
	Vec3  m_worldV;
	float m_radius;
	float m_tubeRadius;
	Vec3  m_worldNormal;
};

// Manages gizmo raycast geometry and registers into RaycastSpace3D.
class GizmoRaycastObject : public RaycastObject
{
public:
	GizmoRaycastObject(GizmoComponent* owner);
	~GizmoRaycastObject();

	// RaycastObject interface
	SceneRaycastResult3D IntersectBounds(RaycastQuery3D const& query) override;
	SceneRaycastResult3D IntersectRay(RaycastQuery3D const& query) override;

	// Called each frame by GizmoComponent to rebuild world-space shapes and AABB.
	void Rebuild(GizmoAxis axis, Vec3 const& originWorld, float scale);

	void ClearShapes();

	GizmoComponent* GetOwner() const { return m_component; }

private:
	GizmoComponent*          m_component = nullptr;
	AABB3                    m_worldAABB;
	std::vector<GizmoShape*> m_shapes;
};
