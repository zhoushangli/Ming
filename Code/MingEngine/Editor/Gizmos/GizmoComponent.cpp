#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Math/AABB3.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/DebugRenderer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Engine/Render/VertexUtils.hpp"

#include <cmath>

namespace
{
float constexpr kGizmoAxisLength        = 1.25f;
float constexpr kGizmoArrowRadius       = 0.035f;
float constexpr kGizmoAxisPickRadius    = 0.11f;
float constexpr kGizmoPlaneOffset       = 0.36f;
float constexpr kGizmoPlaneSize         = 0.22f;
float constexpr kGizmoPlanePickSize     = 0.30f;
float constexpr kGizmoRotationRadius    = 0.88f;
float constexpr kGizmoRotationPickWidth = 0.10f;
int constexpr kGizmoArcSegments         = 32;

Rgba8 const kHoverColor(255, 235, 90, 255);
Rgba8 const kActiveColor(255, 170, 30, 255);
Rgba8 const kGuideColor(220, 220, 220, 255);

float GetRayPointAxisT(Vec3 const& rayStart, Vec3 const& rayForward, Vec3 const& lineOrigin, Vec3 const& lineAxis)
{
	Vec3 const w0           = rayStart - lineOrigin;
	float const a           = DotProduct3D(rayForward, rayForward);
	float const b           = DotProduct3D(rayForward, lineAxis);
	float const c           = DotProduct3D(lineAxis, lineAxis);
	float const d           = DotProduct3D(rayForward, w0);
	float const e           = DotProduct3D(lineAxis, w0);
	float const denominator = a * c - b * b;
	if (Abs(denominator) <= 0.000001f)
	{
		return e / c;
	}

	return (a * e - b * d) / denominator;
}

float GetDistanceRayToSegment(
	Vec3 const& rayStart,
	Vec3 const& rayForward,
	Vec3 const& segmentStart,
	Vec3 const& segmentEnd,
	float& outRayT,
	float& outSegmentT)
{
	Vec3 const segment     = segmentEnd - segmentStart;
	float const segmentLen = segment.GetLength();
	if (segmentLen <= 0.000001f)
	{
		outRayT     = 0.f;
		outSegmentT = 0.f;
		return GetDistance3D(rayStart, segmentStart);
	}

	Vec3 const segmentAxis = segment / segmentLen;
	outSegmentT            = GetRayPointAxisT(rayStart, rayForward, segmentStart, segmentAxis);
	outSegmentT            = GetClamped(outSegmentT, 0.f, segmentLen);
	Vec3 const segmentPos  = segmentStart + segmentAxis * outSegmentT;
	outRayT                = DotProduct3D(segmentPos - rayStart, rayForward);
	outRayT                = Max(0.f, outRayT);
	Vec3 const rayPos      = rayStart + rayForward * outRayT;
	return GetDistance3D(rayPos, segmentPos);
}

bool RaycastPlane(RaycastInfo const& ray, Vec3 const& planePoint, Vec3 const& planeNormal, Vec3& outHit, float& outDist)
{
	float const denominator = DotProduct3D(ray.m_forwardNormal, planeNormal);
	if (Abs(denominator) <= 0.000001f)
	{
		return false;
	}

	outDist = DotProduct3D(planePoint - ray.m_startPos, planeNormal) / denominator;
	if (outDist < 0.f || outDist > ray.m_maxLength)
	{
		return false;
	}

	outHit = ray.m_startPos + ray.m_forwardNormal * outDist;
	return true;
}

void UploadVertices(std::vector<Vertex>& verts, VertexBuffer*& buffer)
{
	if (verts.empty())
	{
		return;
	}

	if (buffer != nullptr)
	{
		delete buffer;
		buffer = nullptr;
	}

	if (g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		buffer = g_engine->m_renderer->CreateVertexBuffer(verts);
	}
}

void AddVertsForTorusArc3D(
	std::vector<Vertex>& verts,
	Vec3 const& origin,
	Vec3 const& u,
	Vec3 const& v,
	float radius,
	float startDegrees,
	float endDegrees,
	float tubeRadius,
	Rgba8 const& color)
{
	Vec3 previous = origin + u * (CosDegrees(startDegrees) * radius) + v * (SinDegrees(startDegrees) * radius);
	for (int segmentIndex = 1; segmentIndex <= kGizmoArcSegments; ++segmentIndex)
	{
		float const t       = (float)segmentIndex / (float)kGizmoArcSegments;
		float const degrees = Interpolate(startDegrees, endDegrees, t);
		Vec3 const current  = origin + u * (CosDegrees(degrees) * radius) + v * (SinDegrees(degrees) * radius);
		AddVertsForCylinder3D(verts, previous, current, tubeRadius, color);
		previous = current;
	}
}

EulerAngles GetEulerWithAxisDelta(EulerAngles const& start, GizmoAxis axis, float degrees)
{
	EulerAngles result = start;
	switch (axis)
	{
	case GizmoAxis::X:
		result.m_rollDegrees += degrees;
		break;
	case GizmoAxis::Y:
		result.m_pitchDegrees += degrees;
		break;
	case GizmoAxis::Z:
		result.m_yawDegrees += degrees;
		break;
	}
	return result;
}
} // namespace

GizmoComponent::GizmoComponent(GizmoAxis axis, Rgba8 const& color) : m_axis(axis), m_baseColor(color) {}

void GizmoComponent::OnBeginDrag(
	[[maybe_unused]] GizmoContext const& context, [[maybe_unused]] GizmoRaycastResult const& hit)
{
	m_isDragging = true;
}

void GizmoComponent::OnDrag([[maybe_unused]] GizmoContext const& context, [[maybe_unused]] RaycastInfo const& ray) {}

void GizmoComponent::OnEndDrag([[maybe_unused]] GizmoContext const& context) { m_isDragging = false; }

void GizmoComponent::SetHovered(bool isHovered) { m_isHovered = isHovered; }

bool GizmoComponent::IsHovered() const { return m_isHovered; }

bool GizmoComponent::IsDragging() const { return m_isDragging; }

bool GizmoComponent::IsRotationGizmo() const { return false; }

Vec3 GizmoComponent::GetAxisWorld() const
{
	switch (m_axis)
	{
	case GizmoAxis::X:
		return Vec3::Forward;
	case GizmoAxis::Y:
		return Vec3::Left;
	case GizmoAxis::Z:
		return Vec3::Up;
	}

	return Vec3::Forward;
}

Vec3 GizmoComponent::GetPlaneU() const
{
	switch (m_axis)
	{
	case GizmoAxis::X:
		return Vec3::Left;
	case GizmoAxis::Y:
		return Vec3::Forward;
	case GizmoAxis::Z:
		return Vec3::Forward;
	}

	return Vec3::Forward;
}

Vec3 GizmoComponent::GetPlaneV() const
{
	switch (m_axis)
	{
	case GizmoAxis::X:
		return Vec3::Up;
	case GizmoAxis::Y:
		return Vec3::Up;
	case GizmoAxis::Z:
		return Vec3::Left;
	}

	return Vec3::Up;
}

Rgba8 GizmoComponent::GetDrawColor() const
{
	if (m_isDragging)
	{
		return kActiveColor;
	}

	if (m_isHovered)
	{
		return kHoverColor;
	}

	return m_baseColor;
}

GizmoAxisArrow::GizmoAxisArrow(GizmoAxis axis, Rgba8 const& color) : GizmoComponent(axis, color) {}

GizmoRaycastResult GizmoAxisArrow::Raycast(GizmoContext const& context, RaycastInfo const& ray)
{
	GizmoRaycastResult result;
	Vec3 const axis     = GetAxisWorld();
	Vec3 const start    = context.m_originWorld + axis * (0.18f * context.m_scale);
	Vec3 const end      = context.m_originWorld + axis * (kGizmoAxisLength * context.m_scale);
	float rayT          = 0.f;
	float axisT         = 0.f;
	float const dist    = GetDistanceRayToSegment(ray.m_startPos, ray.m_forwardNormal, start, end, rayT, axisT);
	float const pickRad = kGizmoAxisPickRadius * context.m_scale;
	if (dist > pickRad || rayT > ray.m_maxLength)
	{
		return result;
	}

	result.m_didImpact  = true;
	result.m_impactDist = rayT;
	result.m_impactPos  = ray.m_startPos + ray.m_forwardNormal * rayT;
	result.m_component  = this;
	return result;
}

void GizmoAxisArrow::RebuildVertices(GizmoContext const& context)
{
	m_verts.clear();

	if (context.m_isRotationActive)
	{
		UploadVertices(m_verts, m_vertexBuffer);
		return;
	}

	Vec3 const axis  = GetAxisWorld();
	Vec3 const start = context.m_originWorld;
	Vec3 const end   = context.m_originWorld + axis * (kGizmoAxisLength * context.m_scale);
	AddVertsForArrow3D(m_verts, start, end, kGizmoArrowRadius * context.m_scale, GetDrawColor());

	UploadVertices(m_verts, m_vertexBuffer);
}

void GizmoAxisArrow::OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit)
{
	GizmoComponent::OnBeginDrag(context, hit);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startPosition = context.m_selectedNode3D->GetWorldPosition();
	m_dragOrigin    = context.m_originWorld;
	m_startAxisT    = DotProduct3D(hit.m_impactPos - m_dragOrigin, GetAxisWorld());
}

void GizmoAxisArrow::OnDrag(GizmoContext const& context, RaycastInfo const& ray)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	Vec3 const axis          = GetAxisWorld();
	float const currentAxisT = GetRayPointAxisT(ray.m_startPos, ray.m_forwardNormal, m_dragOrigin, axis);
	float const delta        = currentAxisT - m_startAxisT;
	context.m_selectedNode3D->SetWorldPosition(m_startPosition + axis * delta);
}

GizmoPlaneSquare::GizmoPlaneSquare(GizmoAxis axis, Rgba8 const& color) : GizmoComponent(axis, color) {}

GizmoRaycastResult GizmoPlaneSquare::Raycast(GizmoContext const& context, RaycastInfo const& ray)
{
	GizmoRaycastResult result;
	Vec3 const normal = GetAxisWorld();
	Vec3 const u      = GetPlaneU();
	Vec3 const v      = GetPlaneV();
	Vec3 const center = context.m_originWorld + (u + v) * (kGizmoPlaneOffset * context.m_scale);
	Vec3 hit;
	float dist = 0.f;
	if (!RaycastPlane(ray, center, normal, hit, dist))
	{
		return result;
	}

	Vec3 const local     = hit - center;
	float const halfSize = kGizmoPlanePickSize * context.m_scale * 0.5f;
	if (Abs(DotProduct3D(local, u)) > halfSize || Abs(DotProduct3D(local, v)) > halfSize)
	{
		return result;
	}

	result.m_didImpact  = true;
	result.m_impactDist = dist;
	result.m_impactPos  = hit;
	result.m_component  = this;
	return result;
}

void GizmoPlaneSquare::RebuildVertices(GizmoContext const& context)
{
	m_verts.clear();

	if (context.m_isRotationActive)
	{
		UploadVertices(m_verts, m_vertexBuffer);
		return;
	}

	Vec3 const u      = GetPlaneU();
	Vec3 const v      = GetPlaneV();
	Vec3 const center = context.m_originWorld + (u + v) * (kGizmoPlaneOffset * context.m_scale);
	float const half  = kGizmoPlaneSize * context.m_scale * 0.5f;
	Rgba8 const color = GetDrawColor();

	Matrix4x4 const localToWorld(u, v, GetAxisWorld(), center);
	Vec3 const minCorner = Vec3(-half, -half, 0.f);
	Vec3 const maxCorner = Vec3(half, half, 0.f);

	// Two triangles forming a square in the plane
	Vec3 const p00 = minCorner;
	Vec3 const p10 = Vec3(maxCorner.x, minCorner.y, 0.f);
	Vec3 const p11 = maxCorner;
	Vec3 const p01 = Vec3(minCorner.x, maxCorner.y, 0.f);

	Vertex v00(p00, color, Vec2(0.f, 0.f));
	Vertex v10(p10, color, Vec2(1.f, 0.f));
	Vertex v11(p11, color, Vec2(1.f, 1.f));
	Vertex v01(p01, color, Vec2(0.f, 1.f));

	m_verts.push_back(v00);
	m_verts.push_back(v10);
	m_verts.push_back(v11);

	m_verts.push_back(v00);
	m_verts.push_back(v11);
	m_verts.push_back(v01);

	TransformVertexArray3D(m_verts, localToWorld);
	UploadVertices(m_verts, m_vertexBuffer);
}

void GizmoPlaneSquare::OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit)
{
	GizmoComponent::OnBeginDrag(context, hit);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startPosition = context.m_selectedNode3D->GetWorldPosition();
	m_startHitWorld = hit.m_impactPos;
}

void GizmoPlaneSquare::OnDrag(GizmoContext const& context, RaycastInfo const& ray)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	Vec3 hit;
	float dist = 0.f;
	if (!RaycastPlane(ray, m_startHitWorld, GetAxisWorld(), hit, dist))
	{
		return;
	}

	context.m_selectedNode3D->SetWorldPosition(m_startPosition + (hit - m_startHitWorld));
}

GizmoRotationArc::GizmoRotationArc(GizmoAxis axis, Rgba8 const& color) : GizmoComponent(axis, color) {}

GizmoRaycastResult GizmoRotationArc::Raycast(GizmoContext const& context, RaycastInfo const& ray)
{
	GizmoRaycastResult result;
	Vec3 hit;
	float dist = 0.f;
	if (!RaycastPlane(ray, context.m_originWorld, GetAxisWorld(), hit, dist))
	{
		return result;
	}

	Vec3 const fromOrigin    = hit - context.m_originWorld;
	float const radius       = fromOrigin.GetLength();
	float const targetRadius = kGizmoRotationRadius * context.m_scale;
	if (Abs(radius - targetRadius) > kGizmoRotationPickWidth * context.m_scale)
	{
		return result;
	}

	result.m_didImpact  = true;
	result.m_impactDist = dist;
	result.m_impactPos  = hit;
	result.m_component  = this;
	return result;
}

void GizmoRotationArc::RebuildVertices(GizmoContext const& context)
{
	m_verts.clear();

	if (context.m_isRotationActive && context.m_activeComponent != this)
	{
		UploadVertices(m_verts, m_vertexBuffer);
		return;
	}

	Vec3 const u           = GetPlaneU();
	Vec3 const v           = GetPlaneV();
	float const radius     = kGizmoRotationRadius * context.m_scale;
	float const lineRadius = kGizmoArrowRadius * 0.45f * context.m_scale;
	Rgba8 const color      = GetDrawColor();

	if (m_isDragging)
	{
		AddVertsForTorusArc3D(m_verts, context.m_originWorld, u, v, radius, 0.f, 360.f, lineRadius, color);
		UploadVertices(m_verts, m_vertexBuffer);
		return;
	}

	AddVertsForTorusArc3D(m_verts, context.m_originWorld, u, v, radius, -125.f, 125.f, lineRadius, color);
	UploadVertices(m_verts, m_vertexBuffer);
}

void GizmoRotationArc::OnBeginDrag(GizmoContext const& context, GizmoRaycastResult const& hit)
{
	GizmoComponent::OnBeginDrag(context, hit);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startOrientation = context.m_selectedNode3D->GetWorldOrientation();
	m_dragOrigin       = context.m_originWorld;
	m_startVectorWorld = (hit.m_impactPos - context.m_originWorld).GetNormalized();
	m_currentHitWorld  = hit.m_impactPos;
	m_currentDegrees   = 0.f;
}

void GizmoRotationArc::OnDrag(GizmoContext const& context, RaycastInfo const& ray)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	Vec3 hit;
	float dist = 0.f;
	if (!RaycastPlane(ray, m_dragOrigin, GetAxisWorld(), hit, dist))
	{
		return;
	}

	Vec3 currentVector = (hit - m_dragOrigin).GetNormalized();
	Vec3 const cross   = CrossProduct3D(m_startVectorWorld, currentVector);
	float const sinVal = DotProduct3D(cross, GetAxisWorld());
	float const cosVal = DotProduct3D(m_startVectorWorld, currentVector);
	m_currentDegrees   = ConvertRadiansToDegrees(atan2f(sinVal, cosVal));
	m_currentHitWorld  = hit;

	context.m_selectedNode3D->SetWorldOrientation(GetEulerWithAxisDelta(m_startOrientation, m_axis, m_currentDegrees));
}

void GizmoRotationArc::OnEndDrag(GizmoContext const& context)
{
	GizmoComponent::OnEndDrag(context);
	m_currentDegrees = 0.f;
}

bool GizmoRotationArc::IsRotationGizmo() const { return true; }
