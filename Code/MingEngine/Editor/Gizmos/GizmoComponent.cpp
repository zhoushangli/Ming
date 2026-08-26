#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

#include <cmath>

using namespace Math;

namespace
{
float constexpr kGizmoAxisLength        = 1.1f;
float constexpr kGizmoArrowRadius       = 0.035f;
float constexpr kGizmoAxisPickRadius    = 0.1f;
float constexpr kGizmoPlaneOffset       = 0.3f;
float constexpr kGizmoPlaneSize         = 0.2f;
float constexpr kGizmoPlanePickSize     = 0.3f;
float constexpr kGizmoRotationRadius    = 0.8f;
float constexpr kGizmoRotationPickWidth = 0.1f;
int constexpr kGizmoArcSegments         = 128;
int constexpr kGizmoArcSectionSegments  = 3;

Color const kHoverColor(255, 235, 90, 255);
Color const kActiveColor(255, 170, 30, 255);
Color const kGuideColor(220, 220, 220, 255);

void AddVertsForRotationRing3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indices,
	Vector3 const&             origin,
	Vector3 const&             axis,
	Vector3 const&             u,
	Vector3 const&             v,
	float                      radius,
	Color const&               color)
{
	unsigned int const vertexBase = static_cast<unsigned int>(verts.size());
	verts.reserve(verts.size() + kGizmoArcSegments * kGizmoArcSectionSegments);
	indices.reserve(indices.size() + kGizmoArcSegments * kGizmoArcSectionSegments * 6);

	for (int arcSegment = 0; arcSegment < kGizmoArcSegments; ++arcSegment)
	{
		float const   arcFraction = static_cast<float>(arcSegment) / static_cast<float>(kGizmoArcSegments);
		float const   arcDegrees  = arcFraction * 360.f;
		float const   arcCos      = CosDegrees(arcDegrees);
		float const   arcSin      = SinDegrees(arcDegrees);
		Vector3 const radial      = u * arcCos + v * arcSin;
		Vector3 const tangent     = -u * arcSin + v * arcCos;
		Vector3 const position    = origin + radial * radius;

		for (int sectionSegment = 0; sectionSegment < kGizmoArcSectionSegments; ++sectionSegment)
		{
			float const sectionFraction =
				static_cast<float>(sectionSegment) / static_cast<float>(kGizmoArcSectionSegments);
			float const   sectionDegrees = sectionFraction * 360.f;
			Vector3 const normal =
				(axis * CosDegrees(sectionDegrees) + radial * SinDegrees(sectionDegrees)).GetNormalized();
			Vector3 const bitangent = CrossProduct3D(normal, tangent).GetNormalized();

			verts.emplace_back(position, color, Vector2(arcFraction, sectionFraction), tangent, bitangent, normal);
		}
	}

	for (int arcSegment = 0; arcSegment < kGizmoArcSegments; ++arcSegment)
	{
		unsigned int const currentRing = vertexBase + arcSegment * kGizmoArcSectionSegments;
		unsigned int const nextRing    = vertexBase + ((arcSegment + 1) % kGizmoArcSegments) * kGizmoArcSectionSegments;

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

} // namespace

GizmoContext BuildGizmoContext(SceneTree* sceneTree, Camera3D const& camera, Vector2 clientPos)
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
		context.m_clientPos        = clientPos;
		context.m_clientDimensions = g_engine && g_engine->m_windowSystem
										 ? (Vector2)g_engine->m_windowSystem->GetClientDimensions()
										 : Vector2::One;
	}

	if (editorNode != nullptr)
	{
		if (editorNode != nullptr)
		{
			context.m_selectedNodeID = editorNode->GetSelection().GetSelected();
			context.m_selectedNode3D = ObjectDatabase::GetInstance<Node3D>(context.m_selectedNodeID);
			if (sceneTree == nullptr
				|| (context.m_selectedNode3D != nullptr && context.m_selectedNode3D->GetSceneTree() != sceneTree))
			{
				context.m_selectedNode3D = nullptr;
			}
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

GizmoComponent::GizmoComponent(GizmoAxis axis, Color const& color) : m_axis(axis), m_baseColor(color)
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

MathRaycastResult3D GizmoComponent::Raycast(
	[[maybe_unused]] GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	return MathRaycastResult3D(raycastInfo.m_startPos, raycastInfo.m_forwardNormal, raycastInfo.m_maxLength);
}

void GizmoComponent::OnBeginDrag([[maybe_unused]] GizmoContext const& context, [[maybe_unused]] Vector3 const& hitPos)
{
	m_isDragging = true;
}

void GizmoComponent::OnDrag(
	[[maybe_unused]] GizmoContext const&       context,
	[[maybe_unused]] MathRaycastQuery3D const& startRaycastInfo,
	[[maybe_unused]] MathRaycastQuery3D const& currentRaycastInfo)
{
}

void GizmoComponent::OnEndDrag([[maybe_unused]] GizmoContext const& context) { m_isDragging = false; }

Vector3 GizmoComponent::GetWorldVirtualCenter() const
{
	Vector3 const worldScale = GetWorldScale();
	Vector3 const scaledVirtualCenter =
		Vector3(m_virtualCenter.x * worldScale.x, m_virtualCenter.y * worldScale.y, m_virtualCenter.z * worldScale.z);
	return scaledVirtualCenter + GetWorldPosition();
}

void GizmoComponent::SetHovered(bool isHovered) { m_isHovered = isHovered; }

bool GizmoComponent::IsHovered() const { return m_isHovered; }

bool GizmoComponent::IsDragging() const { return m_isDragging; }

bool GizmoComponent::IsRotationGizmo() const { return false; }

Vector3 GizmoComponent::GetAxisWorld() const
{
	switch (m_axis)
	{
	case GizmoAxis::X:
		return Vector3::Forward;
	case GizmoAxis::Y:
		return Vector3::Left;
	case GizmoAxis::Z:
		return Vector3::Up;
	}

	return Vector3::Forward;
}

Vector3 GizmoComponent::GetPlaneU() const
{
	switch (m_axis)
	{
	case GizmoAxis::X:
		return Vector3::Left;
	case GizmoAxis::Y:
		return Vector3::Forward;
	case GizmoAxis::Z:
		return Vector3::Forward;
	}

	return Vector3::Forward;
}

Vector3 GizmoComponent::GetPlaneV() const
{
	switch (m_axis)
	{
	case GizmoAxis::X:
		return Vector3::Up;
	case GizmoAxis::Y:
		return Vector3::Up;
	case GizmoAxis::Z:
		return Vector3::Left;
	}

	return Vector3::Up;
}

Color GizmoComponent::GetDrawColor() const
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

GizmoAxisArrow::GizmoAxisArrow(GizmoAxis axis, Color const& color) : GizmoComponent(axis, color)
{
	Vector3 const axisDir = GetAxisWorld();
	Vector3 const start   = Vector3::Zero;
	Vector3 const end     = axisDir * kGizmoAxisLength;
	AddVertsForArrow3D(m_verts, start, end, kGizmoArrowRadius, Color::White);

	m_virtualCenter = axisDir * (kGizmoAxisLength * 0.5f);
}

MathRaycastResult3D GizmoAxisArrow::Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	Vector3 const axisWorld     = GetAxisWorld();
	float const   arrowTipSize  = kGizmoArrowRadius * 4.f;
	float const   pickEndLength = kGizmoAxisLength + arrowTipSize;
	Vector3 const arrowEnd      = context.m_originWorld + axisWorld * (pickEndLength * context.m_scale);
	float const   arrowRadius   = kGizmoAxisPickRadius * context.m_scale;
	return RaycastVsCylinder3D(
		raycastInfo.m_startPos,
		raycastInfo.m_forwardNormal,
		raycastInfo.m_maxLength,
		context.m_originWorld,
		arrowEnd,
		arrowRadius);
}

void GizmoAxisArrow::OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos)
{
	GizmoComponent::OnBeginDrag(context, hitPos);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startPosition = context.m_selectedNode3D->GetWorldPosition();
	m_dragOrigin    = context.m_originWorld;
}

void GizmoAxisArrow::OnDrag(
	GizmoContext const&       context,
	MathRaycastQuery3D const& startRaycastInfo,
	MathRaycastQuery3D const& currentRaycastInfo)
{
	if (context.m_selectedNode3D == nullptr || context.m_camera == nullptr)
	{
		return;
	}

	Vector3 const axis           = GetAxisWorld();
	Vector3 const cameraForward  = context.m_camera->GetWorldOrientation().GetForwardDir_IFwd_JLeft_KUp();
	Vector3 const planeDirection = CrossProduct3D(axis, CrossProduct3D(axis, cameraForward));
	if (planeDirection.GetLengthSquared() <= 1e-6f)
	{
		return;
	}
	Vector3 const planeNormal = planeDirection.GetNormalized();

	MathRaycastResult3D const startHit = RaycastVsPlane3D(
		startRaycastInfo.m_startPos,
		startRaycastInfo.m_forwardNormal,
		startRaycastInfo.m_maxLength,
		m_dragOrigin,
		planeNormal);
	MathRaycastResult3D const currentHit = RaycastVsPlane3D(
		currentRaycastInfo.m_startPos,
		currentRaycastInfo.m_forwardNormal,
		currentRaycastInfo.m_maxLength,
		m_dragOrigin,
		planeNormal);
	if (!startHit.m_didImpact || !currentHit.m_didImpact)
	{
		return;
	}

	Vector3 const dragDelta   = currentHit.m_impactPos - startHit.m_impactPos;
	float const   axisDelta   = DotProduct3D(dragDelta, axis);
	Vector3 const translation = axis * axisDelta;
	context.m_selectedNode3D->SetWorldPosition(m_startPosition + translation);
}

GizmoPlaneSquare::GizmoPlaneSquare(GizmoAxis axis, Color const& color) : GizmoComponent(axis, color)
{
	Vector3 const u      = GetPlaneU();
	Vector3 const v      = GetPlaneV();
	Vector3 const center = (u + v) * kGizmoPlaneOffset;
	float const   half   = kGizmoPlaneSize * 0.5f;

	Matrix4x4 const localToWorld(u, v, GetAxisWorld(), center);
	Vector3 const   minCorner = Vector3(-half, -half, 0.f);
	Vector3 const   maxCorner = Vector3(half, half, 0.f);

	// Two triangles forming a square in the plane
	Vector3 const p00 = minCorner;
	Vector3 const p10 = Vector3(maxCorner.x, minCorner.y, 0.f);
	Vector3 const p11 = maxCorner;
	Vector3 const p01 = Vector3(minCorner.x, maxCorner.y, 0.f);

	Vertex v00(p00, Color::White, Vector2(0.f, 0.f));
	Vertex v10(p10, Color::White, Vector2(1.f, 0.f));
	Vertex v11(p11, Color::White, Vector2(1.f, 1.f));
	Vertex v01(p01, Color::White, Vector2(0.f, 1.f));

	m_verts.push_back(v00);
	m_verts.push_back(v10);
	m_verts.push_back(v11);

	m_verts.push_back(v00);
	m_verts.push_back(v11);
	m_verts.push_back(v01);

	TransformVertexArray3D(m_verts, localToWorld);

	m_virtualCenter = center;
}

MathRaycastResult3D GizmoPlaneSquare::Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	Vector3 const planeU      = GetPlaneU();
	Vector3 const planeV      = GetPlaneV();
	Vector3 const planeCenter = context.m_originWorld + (planeU + planeV) * (kGizmoPlaneOffset * context.m_scale);
	float const   planeHalf   = kGizmoPlanePickSize * context.m_scale * 0.5f;
	Quad3 const   quad(
		planeCenter - planeU * planeHalf - planeV * planeHalf,
		planeCenter + planeU * planeHalf - planeV * planeHalf,
		planeCenter + planeU * planeHalf + planeV * planeHalf,
		planeCenter - planeU * planeHalf + planeV * planeHalf);

	MathRaycastResult3D const result =
		RaycastVsQuad3D(raycastInfo.m_startPos, raycastInfo.m_forwardNormal, raycastInfo.m_maxLength, quad);

	return result;
}

void GizmoPlaneSquare::OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos)
{
	GizmoComponent::OnBeginDrag(context, hitPos);
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	m_startPosition = context.m_selectedNode3D->GetWorldPosition();
	m_dragOrigin    = context.m_originWorld;
}

void GizmoPlaneSquare::OnDrag(
	GizmoContext const&       context,
	MathRaycastQuery3D const& startRaycastInfo,
	MathRaycastQuery3D const& currentRaycastInfo)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	Vector3 const             planeNormal = GetAxisWorld();
	MathRaycastResult3D const startHit    = RaycastVsPlane3D(
		startRaycastInfo.m_startPos,
		startRaycastInfo.m_forwardNormal,
		startRaycastInfo.m_maxLength,
		m_dragOrigin,
		planeNormal);
	MathRaycastResult3D const currentHit = RaycastVsPlane3D(
		currentRaycastInfo.m_startPos,
		currentRaycastInfo.m_forwardNormal,
		currentRaycastInfo.m_maxLength,
		m_dragOrigin,
		planeNormal);
	if (!startHit.m_didImpact || !currentHit.m_didImpact)
	{
		return;
	}

	Vector3 const translation = currentHit.m_impactPos - startHit.m_impactPos;

	context.m_selectedNode3D->SetWorldPosition(m_startPosition + translation);
}

GizmoRotationArc::GizmoRotationArc(GizmoAxis axis, Color const& color) : GizmoComponent(axis, color)
{
	Vector3 const u = GetPlaneU();
	Vector3 const v = GetPlaneV();
	AddVertsForRotationRing3D(
		m_verts,
		m_indices,
		Vector3::Zero,
		GetAxisWorld(),
		u,
		v,
		kGizmoRotationRadius,
		Color::White);

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
	Ref<ShaderResource> shaderResource =
		g_engine->m_renderer->GetBuiltinShaderResource("TransformGizmosArc", BuiltinShaders::TransformGizmosArc);
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
	case NotificationType::Process:
	{
		Vector3 const cameraPos = EditorCamera::Get()->GetWorldPosition();
		Vector3 const gizmoPos  = GetWorldPosition();
		Vector3 const toCamera  = cameraPos - gizmoPos;
		switch (m_axis)
		{
		case GizmoAxis::X:
			m_virtualCenter = Vector3(0.f, toCamera.y, toCamera.z).GetNormalized() * kGizmoRotationRadius;
			break;
		case GizmoAxis::Y:
			m_virtualCenter = Vector3(toCamera.x, 0.f, toCamera.z).GetNormalized() * kGizmoRotationRadius;
			break;
		case GizmoAxis::Z:
			m_virtualCenter = Vector3(toCamera.x, toCamera.y, 0.f).GetNormalized() * kGizmoRotationRadius;
			break;
		}
		break;
	}
	}
}

MathRaycastResult3D GizmoRotationArc::Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	Vector3 const       worldNormal = GetAxisWorld().GetNormalized();
	MathRaycastResult3D result      = RaycastVsPlane3D(
		raycastInfo.m_startPos,
		raycastInfo.m_forwardNormal,
		raycastInfo.m_maxLength,
		context.m_originWorld,
		worldNormal);
	if (!result.m_didImpact)
	{
		return result;
	}

	float const hitRadius = (result.m_impactPos - context.m_originWorld).GetLength();
	float const radius    = kGizmoRotationRadius * context.m_scale;
	float const pickWidth = kGizmoRotationPickWidth * context.m_scale;
	if (Abs(hitRadius - radius) > pickWidth)
	{
		return MathRaycastResult3D(raycastInfo.m_startPos, raycastInfo.m_forwardNormal, raycastInfo.m_maxLength);
	}

	if (DotProduct3D(result.m_impactNormal, raycastInfo.m_forwardNormal) > 0.f)
	{
		result.m_impactNormal = -result.m_impactNormal;
	}
	return result;
}

void GizmoRotationArc::OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos)
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

void GizmoRotationArc::OnDrag(
	GizmoContext const&                        context,
	[[maybe_unused]] MathRaycastQuery3D const& startRaycastInfo,
	MathRaycastQuery3D const&                  currentRaycastInfo)
{
	if (context.m_selectedNode3D == nullptr)
	{
		return;
	}

	MathRaycastResult3D const hit = RaycastVsPlane3D(
		currentRaycastInfo.m_startPos,
		currentRaycastInfo.m_forwardNormal,
		currentRaycastInfo.m_maxLength,
		m_dragOrigin,
		GetAxisWorld());
	if (!hit.m_didImpact)
	{
		return;
	}

	Vector3       currentVector = (hit.m_impactPos - m_dragOrigin).GetNormalized();
	Vector3 const cross         = CrossProduct3D(m_startVectorWorld, currentVector);
	float const   sinVal        = DotProduct3D(cross, GetAxisWorld());
	float const   cosVal        = DotProduct3D(m_startVectorWorld, currentVector);
	m_currentDegrees            = ConvertRadiansToDegrees(atan2f(sinVal, cosVal));

	context.m_selectedNode3D->SetWorldOrientation(GetEulerWithAxisDelta(m_startOrientation, m_axis, m_currentDegrees));
}

void GizmoRotationArc::OnEndDrag(GizmoContext const& context)
{
	GizmoComponent::OnEndDrag(context);
	m_currentDegrees = 0.f;
}

bool GizmoRotationArc::IsRotationGizmo() const { return true; }
