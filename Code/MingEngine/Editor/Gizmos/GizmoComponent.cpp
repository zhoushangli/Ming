#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/GizmoRaycastObject.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

#include <cmath>

using namespace Math;

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
int constexpr kGizmoArcSegments         = 64;
int constexpr kGizmoArcSectionSegments  = 3;

Rgba8 const kHoverColor(255, 235, 90, 255);
Rgba8 const kActiveColor(255, 170, 30, 255);
Rgba8 const kGuideColor(220, 220, 220, 255);

Rgba8 const kDebugRaycastColor = Rgba8::Cyan;

void AddVertsForRotationRing3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indices,
	Vec3 const&                origin,
	Vec3 const&                axis,
	Vec3 const&                u,
	Vec3 const&                v,
	float                      radius,
	Rgba8 const&               color)
{
	unsigned int const vertexBase = static_cast<unsigned int>(verts.size());
	verts.reserve(verts.size() + kGizmoArcSegments * kGizmoArcSectionSegments);
	indices.reserve(indices.size() + kGizmoArcSegments * kGizmoArcSectionSegments * 6);

	for (int arcSegment = 0; arcSegment < kGizmoArcSegments; ++arcSegment)
	{
		float const arcFraction = static_cast<float>(arcSegment) / static_cast<float>(kGizmoArcSegments);
		float const arcDegrees  = arcFraction * 360.f;
		float const arcCos      = CosDegrees(arcDegrees);
		float const arcSin      = SinDegrees(arcDegrees);
		Vec3 const  radial      = u * arcCos + v * arcSin;
		Vec3 const  tangent     = -u * arcSin + v * arcCos;
		Vec3 const  position    = origin + radial * radius;

		for (int sectionSegment = 0; sectionSegment < kGizmoArcSectionSegments; ++sectionSegment)
		{
			float const sectionFraction =
				static_cast<float>(sectionSegment) / static_cast<float>(kGizmoArcSectionSegments);
			float const sectionDegrees = sectionFraction * 360.f;
			Vec3 const normal =
				(axis * CosDegrees(sectionDegrees) + radial * SinDegrees(sectionDegrees)).GetNormalized();
			Vec3 const bitangent = CrossProduct3D(normal, tangent).GetNormalized();

			verts.emplace_back(position, color, Vec2(arcFraction, sectionFraction), tangent, bitangent, normal);
		}
	}

	for (int arcSegment = 0; arcSegment < kGizmoArcSegments; ++arcSegment)
	{
		unsigned int const currentRing = vertexBase + arcSegment * kGizmoArcSectionSegments;
		unsigned int const nextRing =
			vertexBase + ((arcSegment + 1) % kGizmoArcSegments) * kGizmoArcSectionSegments;

		for (int sectionSegment = 0; sectionSegment < kGizmoArcSectionSegments; ++sectionSegment)
		{
			unsigned int const currentSection = sectionSegment;
			unsigned int const nextSection    = (sectionSegment + 1) % kGizmoArcSectionSegments;

			indices.push_back(currentRing + nextSection);
			indices.push_back(currentRing + currentSection);
			indices.push_back(nextRing + currentSection);

			indices.push_back(nextRing + currentSection);
			indices.push_back(nextRing + nextSection);
			indices.push_back(currentRing + nextSection);
		}
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

AABB3 GetSegmentAABB(Vec3 const& start, Vec3 const& end, float radius)
{
	Vec3 const mins = Vec3(Min(start.x, end.x) - radius, Min(start.y, end.y) - radius, Min(start.z, end.z) - radius);
	Vec3 const maxs = Vec3(Max(start.x, end.x) + radius, Max(start.y, end.y) + radius, Max(start.z, end.z) + radius);
	return AABB3(mins, maxs);
}
} // namespace

GizmoContext BuildGizmoContext(SceneTree* sceneTree, Camera3D const& camera, Vec2 clientPos)
{
	GizmoContext context;
	context.m_sceneTree = sceneTree;
	context.m_camera    = &camera;

	// 1) Convert window-space mouse to viewport-space via EditorUI
	// 2) EditorUI rect is updated every frame by ViewportPanel
	EditorNode* editorNode = EditorNode::Get();
	if (editorNode != nullptr && editorNode->m_editorUI != nullptr)
	{
		context.m_clientPos        = editorNode->m_editorUI->ToViewportPos(clientPos);
		context.m_clientDimensions = editorNode->m_editorUI->GetViewportDimensions();
	}
	else
	{
		context.m_clientPos = clientPos;
		context.m_clientDimensions =
			g_engine && g_engine->m_windowSystem ? (Vec2)g_engine->m_windowSystem->GetClientDimensions() : Vec2::One;
	}

	if (editorNode != nullptr)
	{
		if (editorNode != nullptr)
		{
			context.m_selectedNode = editorNode->GetSelection().GetSelected();
			context.m_selectedNode3D =
				dynamic_cast<Node3D*>(sceneTree != nullptr ? sceneTree->ResolveNode(context.m_selectedNode) : nullptr);
		}
	}

	if (context.m_selectedNode3D != nullptr)
	{
		context.m_originWorld  = context.m_selectedNode3D->GetWorldPosition();
		float const cameraDist = (context.m_originWorld - camera.GetWorldPosition()).GetLength();
		context.m_scale        = Max(0.1f, cameraDist) * 0.15f;
	}

	return context;
}

GizmoComponent::GizmoComponent(GizmoAxis axis, Rgba8 const& color) : m_axis(axis), m_baseColor(color)
{
	SetReady(true);
	SetProcess(true);
}

RenderRequest GizmoComponent::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass           = RenderRequestPass::Opaque;
	request.m_renderPriority = m_renderPriority; // Gizmos render after most other objects
	request.m_modelToWorld   = GetWorldTransform();
	request.m_vertexBuffer   = m_vertexBuffer;
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::DISABLED;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	request.m_tint           = GetDrawColor();
	return request;
}

void GizmoComponent::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::Ready:
	{
		if (!m_verts.empty() && m_vertexBuffer == nullptr && g_engine != nullptr && g_engine->m_renderer != nullptr)
		{
			unsigned int const vertexBufferSize = static_cast<unsigned int>(m_verts.size() * sizeof(Vertex));
			m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_verts.data(), vertexBufferSize, sizeof(Vertex));
		}
		break;
	}
	}
}

void GizmoComponent::UpdateRaycastObject([[maybe_unused]] GizmoContext const& context) {}

void GizmoComponent::OnBeginDrag([[maybe_unused]] GizmoContext const& context, [[maybe_unused]] Vec3 const& hitPos)
{
	m_isDragging = true;
}

void GizmoComponent::OnDrag(
	[[maybe_unused]] GizmoContext const& context,
	[[maybe_unused]] Vec3 const&         rayStart,
	[[maybe_unused]] Vec3 const&         rayFwdNormal)
{
}

void GizmoComponent::OnEndDrag([[maybe_unused]] GizmoContext const& context) { m_isDragging = false; }

Vec3 GizmoComponent::GetWorldVirtualCenter() const
{
	Vec3 const worldScale = GetWorldScale();
	Vec3 const scaledVirtualCenter =
		Vec3(m_virtualCenter.x * worldScale.x, m_virtualCenter.y * worldScale.y, m_virtualCenter.z * worldScale.z);
	return scaledVirtualCenter + GetWorldPosition();
}

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

GizmoAxisArrow::GizmoAxisArrow(GizmoAxis axis, Rgba8 const& color) : GizmoComponent(axis, color)
{
	Vec3 const axisDir = GetAxisWorld();
	Vec3 const start   = Vec3::Zero;
	Vec3 const end     = axisDir * kGizmoAxisLength;
	AddVertsForArrow3D(m_verts, start, end, kGizmoArrowRadius, Rgba8::White);

	m_virtualCenter = axisDir * (kGizmoAxisLength * 0.5f);
}

void GizmoAxisArrow::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr && m_raycastObject == nullptr)
		{
			m_raycastObject          = new GizmoAxisRaycastObject(this);
			m_raycastObject->m_owner = GetHandle();
			raycastSpace->AddObject(m_raycastObject);
		}
		break;
	}
	case NotificationType::ExitTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr && m_raycastObject != nullptr)
		{
			raycastSpace->RemoveObject(m_raycastObject);
			delete m_raycastObject;
			m_raycastObject = nullptr;
		}
		break;
	}
	}
}

void GizmoAxisArrow::UpdateRaycastObject(GizmoContext const& context)
{
	GizmoAxisRaycastObject* raycastObject = dynamic_cast<GizmoAxisRaycastObject*>(m_raycastObject);
	if (raycastObject == nullptr)
	{
		return;
	}

	Vec3 const  axisWorld     = GetAxisWorld();
	float const arrowTipSize  = kGizmoArrowRadius * 4.f;
	float const pickEndLength = kGizmoAxisLength + arrowTipSize;

	raycastObject->m_arrowStart = context.m_originWorld;
	// Axis pick cylinder covers both the shaft and arrow tip
	// e.g. PickEnd = Origin + Axis * (AxisLength + ArrowTipLength)
	raycastObject->m_arrowEnd = context.m_originWorld + axisWorld * (pickEndLength * context.m_scale);
	raycastObject->m_arrowRad = kGizmoAxisPickRadius * context.m_scale;
	raycastObject->m_worldAABB =
		GetSegmentAABB(raycastObject->m_arrowStart, raycastObject->m_arrowEnd, raycastObject->m_arrowRad);

	// DebugGizmos::AddWorldWireCylinder(
	// 	raycastObject->m_arrowStart,
	// 	raycastObject->m_arrowEnd,
	// 	raycastObject->m_arrowRad,
	// 	0.f,
	// 	kDebugRaycastColor,
	// 	kDebugRaycastColor,
	// 	DebugRenderMode::X_RAY);
}

void GizmoAxisArrow::OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos)
{
	GizmoComponent::OnBeginDrag(context, hitPos);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startPosition = context.m_selectedNode3D->GetWorldPosition();
	m_dragOrigin    = context.m_originWorld;
	m_startAxisT    = DotProduct3D(hitPos - m_dragOrigin, GetAxisWorld());
}

void GizmoAxisArrow::OnDrag(GizmoContext const& context, Vec3 const& rayStart, Vec3 const& rayFwdNormal)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	Vec3 const  axis         = GetAxisWorld();
	Vec3 const  w0           = rayStart - m_dragOrigin;
	float const a            = DotProduct3D(rayFwdNormal, rayFwdNormal);
	float const b            = DotProduct3D(rayFwdNormal, axis);
	float const c            = DotProduct3D(axis, axis);
	float const d            = DotProduct3D(rayFwdNormal, w0);
	float const e            = DotProduct3D(axis, w0);
	float const denominator  = a * c - b * b;
	float const currentAxisT = Abs(denominator) <= 0.000001f ? e / c : (a * e - b * d) / denominator;
	float const delta        = currentAxisT - m_startAxisT;
	context.m_selectedNode3D->SetWorldPosition(m_startPosition + axis * delta);
}

GizmoPlaneSquare::GizmoPlaneSquare(GizmoAxis axis, Rgba8 const& color) : GizmoComponent(axis, color)
{
	Vec3 const  u      = GetPlaneU();
	Vec3 const  v      = GetPlaneV();
	Vec3 const  center = (u + v) * kGizmoPlaneOffset;
	float const half   = kGizmoPlaneSize * 0.5f;

	Matrix4x4 const localToWorld(u, v, GetAxisWorld(), center);
	Vec3 const      minCorner = Vec3(-half, -half, 0.f);
	Vec3 const      maxCorner = Vec3(half, half, 0.f);

	// Two triangles forming a square in the plane
	Vec3 const p00 = minCorner;
	Vec3 const p10 = Vec3(maxCorner.x, minCorner.y, 0.f);
	Vec3 const p11 = maxCorner;
	Vec3 const p01 = Vec3(minCorner.x, maxCorner.y, 0.f);

	Vertex v00(p00, Rgba8::White, Vec2(0.f, 0.f));
	Vertex v10(p10, Rgba8::White, Vec2(1.f, 0.f));
	Vertex v11(p11, Rgba8::White, Vec2(1.f, 1.f));
	Vertex v01(p01, Rgba8::White, Vec2(0.f, 1.f));

	m_verts.push_back(v00);
	m_verts.push_back(v10);
	m_verts.push_back(v11);

	m_verts.push_back(v00);
	m_verts.push_back(v11);
	m_verts.push_back(v01);

	TransformVertexArray3D(m_verts, localToWorld);

	m_virtualCenter = center;
}

void GizmoPlaneSquare::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr && m_raycastObject == nullptr)
		{
			m_raycastObject          = new GizmoPlaneRaycastObject(this);
			m_raycastObject->m_owner = GetHandle();
			raycastSpace->AddObject(m_raycastObject);
		}
		break;
	}
	case NotificationType::ExitTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr && m_raycastObject != nullptr)
		{
			raycastSpace->RemoveObject(m_raycastObject);
			delete m_raycastObject;
			m_raycastObject = nullptr;
		}
		break;
	}
	}
}

void GizmoPlaneSquare::UpdateRaycastObject(GizmoContext const& context)
{
	GizmoPlaneRaycastObject* raycastObject = dynamic_cast<GizmoPlaneRaycastObject*>(m_raycastObject);
	if (raycastObject == nullptr)
	{
		return;
	}

	Vec3 const  planeU      = GetPlaneU();
	Vec3 const  planeV      = GetPlaneV();
	Vec3 const  planeCenter = context.m_originWorld + (planeU + planeV) * (kGizmoPlaneOffset * context.m_scale);
	float const planeHalf   = kGizmoPlanePickSize * context.m_scale * 0.5f;

	raycastObject->m_quad = Quad3(
		planeCenter - planeU * planeHalf - planeV * planeHalf,
		planeCenter + planeU * planeHalf - planeV * planeHalf,
		planeCenter + planeU * planeHalf + planeV * planeHalf,
		planeCenter - planeU * planeHalf + planeV * planeHalf);

	raycastObject->m_worldAABB = AABB3(raycastObject->m_quad.m_points[0], raycastObject->m_quad.m_points[0]);
	for (int pointIndex = 1; pointIndex < 4; ++pointIndex)
	{
		raycastObject->m_worldAABB.StretchToIncludePoint(raycastObject->m_quad.m_points[pointIndex]);
	}

	// DebugGizmos::AddWorldWireAABB(
	// 	raycastObject->m_worldAABB,
	// 	0.f,
	// 	kDebugRaycastColor,
	// 	kDebugRaycastColor,
	// 	DebugRenderMode::X_RAY);
}

void GizmoPlaneSquare::OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos)
{
	GizmoComponent::OnBeginDrag(context, hitPos);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startPosition = context.m_selectedNode3D->GetWorldPosition();
	m_startHitWorld = hitPos;
}

void GizmoPlaneSquare::OnDrag(GizmoContext const& context, Vec3 const& rayStart, Vec3 const& rayFwdNormal)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	MathRaycastResult3D const hit = RaycastVsPlane3D(rayStart, rayFwdNormal, 10000.f, m_startHitWorld, GetAxisWorld());
	if (!hit.m_didImpact)
	{
		return;
	}

	context.m_selectedNode3D->SetWorldPosition(m_startPosition + (hit.m_impactPos - m_startHitWorld));
}

GizmoRotationArc::GizmoRotationArc(GizmoAxis axis, Rgba8 const& color) : GizmoComponent(axis, color)
{
	Vec3 const u = GetPlaneU();
	Vec3 const v = GetPlaneV();
	AddVertsForRotationRing3D(
		m_verts, m_indices, Vec3::Zero, GetAxisWorld(), u, v, kGizmoRotationRadius, Rgba8::White);

	m_virtualCenter = (u + v).GetNormalized() * 0.5f * kGizmoRotationRadius;
}

GizmoRotationArc::~GizmoRotationArc()
{
	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

RenderRequest GizmoRotationArc::SubmitRenderRequest() const
{
	if (m_vertexBuffer == nullptr || m_indexBuffer == nullptr)
	{
		return RenderRequest();
	}

	RenderRequest request = GizmoComponent::SubmitRenderRequest();
	request.m_indexBuffer = m_indexBuffer;
	Ref<ShaderResource> shaderResource(ResourceLoader::Load("res://Shaders/TransformGizmosArc.hlsl"));
	request.m_shader = shaderResource.IsValid() ? shaderResource->GetShader() : nullptr;
	return request;
}

void GizmoRotationArc::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::Ready:
	{
		if (!m_indices.empty() && m_indexBuffer == nullptr && g_engine != nullptr && g_engine->m_renderer != nullptr)
		{
			unsigned int const indexBufferSize = static_cast<unsigned int>(m_indices.size() * sizeof(unsigned int));
			m_indexBuffer =
				g_engine->m_renderer->CreateIndexBuffer(m_indices.data(), indexBufferSize, sizeof(unsigned int));
		}
		break;
	}
	case NotificationType::EnterTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr && m_raycastObject == nullptr)
		{
			m_raycastObject          = new GizmoArcRaycastObject(this);
			m_raycastObject->m_owner = GetHandle();
			raycastSpace->AddObject(m_raycastObject);
		}
		break;
	}
	case NotificationType::ExitTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr && m_raycastObject != nullptr)
		{
			raycastSpace->RemoveObject(m_raycastObject);
			delete m_raycastObject;
			m_raycastObject = nullptr;
		}
		break;
	}
	case NotificationType::Process:
	{
		Vec3 const cameraPos = EditorCamera::Get()->GetWorldPosition();
		Vec3 const gizmoPos  = GetWorldPosition();
		Vec3 const toCamera  = cameraPos - gizmoPos;
		switch (m_axis)
		{
		case GizmoAxis::X:
			m_virtualCenter = Vec3(0.f, toCamera.y, toCamera.z).GetNormalized() * kGizmoRotationRadius;
			break;
		case GizmoAxis::Y:
			m_virtualCenter = Vec3(toCamera.x, 0.f, toCamera.z).GetNormalized() * kGizmoRotationRadius;
			break;
		case GizmoAxis::Z:
			m_virtualCenter = Vec3(toCamera.x, toCamera.y, 0.f).GetNormalized() * kGizmoRotationRadius;
			break;
		}
		break;
	}
	}
}

void GizmoRotationArc::UpdateRaycastObject(GizmoContext const& context)
{
	GizmoArcRaycastObject* raycastObject = dynamic_cast<GizmoArcRaycastObject*>(m_raycastObject);
	if (raycastObject == nullptr)
	{
		return;
	}

	raycastObject->m_worldOrigin = context.m_originWorld;
	raycastObject->m_worldU      = GetPlaneU();
	raycastObject->m_worldV      = GetPlaneV();
	raycastObject->m_radius      = kGizmoRotationRadius * context.m_scale;
	raycastObject->m_tubeRadius  = kGizmoRotationPickWidth * context.m_scale;
	raycastObject->m_worldNormal = GetAxisWorld().GetNormalized();

	float const outer          = raycastObject->m_radius + raycastObject->m_tubeRadius;
	raycastObject->m_worldAABB = AABB3(
		raycastObject->m_worldOrigin - Vec3(outer, outer, outer),
		raycastObject->m_worldOrigin + Vec3(outer, outer, outer));

	// DebugGizmos::AddWorldWireAABB(
	// 	raycastObject->m_worldAABB,
	// 	0.f,
	// 	kDebugRaycastColor,
	// 	kDebugRaycastColor,
	// 	DebugRenderMode::X_RAY);
}

void GizmoRotationArc::OnBeginDrag(GizmoContext const& context, Vec3 const& hitPos)
{
	GizmoComponent::OnBeginDrag(context, hitPos);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startOrientation = context.m_selectedNode3D->GetWorldOrientation();
	m_dragOrigin       = context.m_originWorld;
	m_startVectorWorld = (hitPos - context.m_originWorld).GetNormalized();
	m_currentDegrees   = 0.f;
}

void GizmoRotationArc::OnDrag(GizmoContext const& context, Vec3 const& rayStart, Vec3 const& rayFwdNormal)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	MathRaycastResult3D const hit = RaycastVsPlane3D(rayStart, rayFwdNormal, 10000.f, m_dragOrigin, GetAxisWorld());
	if (!hit.m_didImpact)
	{
		return;
	}

	Vec3        currentVector = (hit.m_impactPos - m_dragOrigin).GetNormalized();
	Vec3 const  cross         = CrossProduct3D(m_startVectorWorld, currentVector);
	float const sinVal        = DotProduct3D(cross, GetAxisWorld());
	float const cosVal        = DotProduct3D(m_startVectorWorld, currentVector);
	m_currentDegrees          = ConvertRadiansToDegrees(atan2f(sinVal, cosVal));

	context.m_selectedNode3D->SetWorldOrientation(GetEulerWithAxisDelta(m_startOrientation, m_axis, m_currentDegrees));
}

void GizmoRotationArc::OnEndDrag(GizmoContext const& context)
{
	GizmoComponent::OnEndDrag(context);
	m_currentDegrees = 0.f;
}

bool GizmoRotationArc::IsRotationGizmo() const { return true; }
