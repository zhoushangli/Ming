#include "MingEngine/Editor/Gizmos/GizmoComponent.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Core/Math/AABB3.hpp"
#include <algorithm>
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"

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

Matrix4x4 AxisRotation(Vector3 const& axis, float degrees)
{
	float const c      = CosDegrees(degrees);
	float const sn     = SinDegrees(degrees);
	auto        rotate = [&](Vector3 const& v)
	{
		return v * c + CrossProduct3D(axis, v) * sn + axis * DotProduct3D(axis, v) * (1.f - c);
	};
	return Matrix4x4(rotate(Vector3::Forward), rotate(Vector3::Left), rotate(Vector3::Up), Vector3::Zero);
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
		context.m_selectedNodeID = editorNode->GetSelection().GetSelected();
		context.m_selectedNode3D = ObjectDatabase::GetInstance<Node3D>(context.m_selectedNodeID);
		if (context.m_selectedNode3D != nullptr
			&& (context.m_selectedNode3D->GetSceneTree() != sceneTree || context.m_selectedNode3D->IsQueuedForDeletion()
				|| !context.m_selectedNode3D->GetSerializable()))
		{
			context.m_selectedNode3D = nullptr;
		}

		if (editorNode->m_editorGizmos != nullptr)
		{
			context.m_tool  = editorNode->m_editorGizmos->GetTool();
			context.m_local = editorNode->m_editorGizmos->IsLocal();
		}
	}

	if (context.m_selectedNode3D != nullptr)
	{
		context.m_originWorld = context.m_selectedNode3D->GetWorldPosition();
		if (context.m_local)
		{
			context.m_basis = context.m_selectedNode3D->GetWorldOrientation().GetAsMatrix_IFwd_JLeft_KUp();
		}

		if (camera.GetMode() == CameraMode::Perspective)
		{
			Vector3 const direction = (context.m_originWorld - camera.GetWorldPosition()).GetNormalized();
			context.m_planeSigns = Vector3(
				DotProduct3D(context.m_basis.GetIBasis3D(), direction) > 1e-6f ? -1.f : 1.f,
				DotProduct3D(context.m_basis.GetJBasis3D(), direction) > 1e-6f ? -1.f : 1.f,
				DotProduct3D(context.m_basis.GetKBasis3D(), direction) > 1e-6f ? -1.f : 1.f);
		}

		float const height = std::max(1.f, context.m_clientDimensions.y);
		float const depth  = DotProduct3D(context.m_originWorld - camera.GetWorldPosition(), camera.GetWorldForward());
		float const extent = camera.GetMode() == CameraMode::Orthographic
								 ? camera.GetSize()
								 : 2.f * std::max(camera.GetNearClip(), depth)
									   * std::tan(ConvertDegreesToRadians(camera.GetFovDegrees() * 0.5f));
		float const pixels = editorNode != nullptr
								 ? static_cast<float>(editorNode->GetEditorTheme()->GetConstant("gizmo_size", "Editor"))
								 : 80.f;
		context.m_scale    = std::max(0.0001f, extent * pixels / height * std::min(1.f, height / (pixels * 4.f)));
	}

	return context;
}

GizmoComponent::GizmoComponent(GizmoAxis axis, Color const& color) : m_axis(axis), m_baseColor(color)
{
	SetReady(true);
	SetProcess(true);
}

void GizmoComponent::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Ready:
		UploadGeometry(
			IsRotationGizmo() ? "TransformGizmosArc" : "DefaultUnlit",
			IsRotationGizmo() ? BuiltinShaders::TransformGizmosArc : BuiltinShaders::DefaultUnlit);
		SetGizmoColor(GetDrawColor());
		break;
	}
}

MathRaycastResult3D GizmoComponent::Raycast(
	[[maybe_unused]] GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	return MathRaycastResult3D(raycastInfo.m_startPos, raycastInfo.m_forwardNormal, raycastInfo.m_maxLength);
}

void GizmoComponent::OnBeginDrag(GizmoContext const& context, Vector3 const& hitPos)
{
	m_isDragging             = true;
	m_feedbackStartDirection = (hitPos - context.m_originWorld).GetNormalized();
	SetGizmoColor(GetDrawColor());
	UpdateDragFeedback(context, m_kind == GizmoKind::Scale || m_kind == GizmoKind::UniformScale ? 1.f : 0.f);
}

void GizmoComponent::OnDrag(
	[[maybe_unused]] GizmoContext const&       context,
	[[maybe_unused]] MathRaycastQuery3D const& startRaycastInfo,
	[[maybe_unused]] MathRaycastQuery3D const& currentRaycastInfo)
{
}

void GizmoComponent::OnEndDrag([[maybe_unused]] GizmoContext const& context)
{
	m_isDragging = false;
	SetGizmoColor(GetDrawColor());
	if (m_dragVisual != nullptr)
	{
		m_dragVisual->SetVisible(false);
		m_dragLabel->SetVisible(false);
	}
}

Vector3 GizmoComponent::GetWorldVirtualCenter() const
{
	return GetWorldTransform().TransformPosition3D(m_virtualCenter);
}

void GizmoComponent::SetHovered(bool isHovered)
{
	m_isHovered = isHovered;
	SetGizmoColor(GetDrawColor());
}

void GizmoComponent::RefreshDragFeedback(GizmoContext const& context)
{
	UpdateDragFeedback(context, m_feedbackValue);
}

void GizmoComponent::UpdateView(GizmoContext const& context)
{
	Vector3 signs = Vector3::One;
	if (m_kind == GizmoKind::Plane)
	{
		signs = context.m_planeSigns;
		float const parity = signs.x * signs.y * signs.z;

		// Keep plane mirroring a proper rotation so child feedback can cancel the parent transform.
		// e.g. Mirror X on an XY handle by also reversing its unused Z normal.
		signs = signs * (Vector3::One + GetAxisWorld() * (parity - 1.f));
	}

	EulerAngles orientation;
	orientation.SetFromMatrix_IFwd_JLeft_KUp(context.m_basis);
	SetWorldPosition(context.m_originWorld);
	SetWorldOrientation(orientation);
	SetWorldScale(signs * context.m_scale);
}

bool GizmoComponent::IsHovered() const
{
	return m_isHovered;
}

bool GizmoComponent::IsDragging() const
{
	return m_isDragging;
}

bool GizmoComponent::IsRotationGizmo() const
{
	return false;
}

bool GizmoComponent::IsEnabledForTool(GizmoTool tool) const
{
	switch (m_kind)
	{
	case GizmoKind::Scale:
	case GizmoKind::UniformScale:
		return tool == GizmoTool::Scale;
	case GizmoKind::Rotate:
		return tool == GizmoTool::Transform || tool == GizmoTool::Rotate;
	default:
		return tool == GizmoTool::Transform || tool == GizmoTool::Move;
	}
}

Vector3 GizmoComponent::GetContextAxis(GizmoContext const& context) const
{
	return context.m_basis.TransformDirection3D(GetAxisWorld()).GetNormalized();
}

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
	Color color = m_baseColor;
	if (m_kind == GizmoKind::Plane)
	{
		if (m_isHovered || m_isDragging)
		{
			float const maximum = std::max(1.f, static_cast<float>(std::max({ color.r, color.g, color.b })));
			auto        brighten = [&](unsigned char channel)
			{
				return static_cast<unsigned char>(255.f * (0.75f + 0.25f * channel / maximum));
			};

			return Color(brighten(color.r), brighten(color.g), brighten(color.b), 255);
		}

		color.a = 230;
		return color;
	}

	if (m_isDragging)
	{
		color = kActiveColor;
	}
	else if (m_isHovered)
	{
		color = kHoverColor;
	}

	return color;
}

void GizmoComponent::UpdateDragFeedback(GizmoContext const& context, float value)
{
	m_feedbackValue = value;
	if (GetSceneTree() == nullptr || context.m_selectedNode3D == nullptr || context.m_camera == nullptr)
	{
		return;
	}

	if (m_dragVisual == nullptr)
	{
		m_dragVisual = MemNew<EditorGizmoVisual3D>();
		m_dragVisual->SetName("GizmoDragFeedback");
		m_dragVisual->SetSerializable(false);
		AddNode(m_dragVisual);
		m_dragVisual->SetRenderPriority(1100);
		m_dragLabel = MemNew<Label>();
		m_dragLabel->SetName("GizmoDragValue");
		m_dragLabel->SetSerializable(false);
		AddNode(m_dragLabel);
		m_dragLabel->SetThemeSource(EditorNode::Get()->m_editorUI->GetRootControl());
		m_dragLabel->SetFontColor(kActiveColor);
	}

	std::vector<Vertex> vertices;
	Vector3 const       origin    = context.m_originWorld;
	Vector3 const       axis      = GetContextAxis(context);
	GizmoContext const  view      = BuildGizmoContext(context.m_sceneTree, *context.m_camera);
	float const         scale     = view.m_scale;
	float const         thickness = scale * 0.006f;
	auto                line      = [&](Vector3 const& start, Vector3 const& end, Color const& color)
	{
		if ((end - start).GetLengthSquared() > 1e-10f)
		{
			AddVertsForCylinder3D(vertices, start, end, thickness, color, AABB2::Unit, 6);
		}
	};

	switch (m_kind)
	{
	case GizmoKind::Rotate:
	{
		Vector3 const start    = m_feedbackStartDirection;
		float const   radius   = kGizmoRotationRadius * scale;
		int const     segments = std::max(1, static_cast<int>(std::ceil(std::abs(value) / 4.f)));
		Vector3       previous = origin + start * radius;
		Color         fill     = kActiveColor;
		fill.a                 = 55;
		for (int i = 1; i <= segments; ++i)
		{
			float const   degrees = value * static_cast<float>(i) / segments;
			Vector3 const current = origin + AxisRotation(axis, degrees).TransformDirection3D(start) * radius;
			vertices.emplace_back(origin, fill);
			vertices.emplace_back(previous, fill);
			vertices.emplace_back(current, fill);
			previous = current;
		}

		line(origin, origin + start * radius, kGuideColor);
		line(origin, previous, kActiveColor);
		m_dragLabel->SetText(Stringf("%.1f deg", value));
		break;
	}
	case GizmoKind::UniformScale:
	{
		Matrix4x4 const cameraBasis = context.m_camera->GetWorldOrientation().GetAsMatrix_IFwd_JLeft_KUp();
		Vector3 const   u           = cameraBasis.GetJBasis3D();
		Vector3 const   v           = cameraBasis.GetKBasis3D();
		for (float factor : { 1.f, value })
		{
			float const   radius    = scale * 0.14f * factor;
			Vector3 const corners[] = { origin + (u + v) * radius,
									   origin + (-u + v) * radius,
									   origin + (-u - v) * radius,
									   origin + (u - v) * radius };
			for (int i = 0; i < 4; ++i)
			{
				line(corners[i], corners[(i + 1) % 4], factor == 1.f ? kGuideColor : kActiveColor);
			}
		}

		m_dragLabel->SetText(Stringf("x %.3f", value));
		break;
	}
	case GizmoKind::Scale:
	{
		Vector3 const end  = origin + axis * (scale * kGizmoAxisLength);
		Vector3 const tick = context.m_basis.TransformDirection3D(GetPlaneU()) * (scale * 0.08f);
		line(origin, end, kGuideColor);
		line(end - tick, end + tick, kGuideColor);
		line(origin, origin + (end - origin) * value, kActiveColor);
		m_dragLabel->SetText(Stringf("x %.3f", value));
		break;
	}
	case GizmoKind::Move:
	{
		float const extent = scale * 100.f;
		Color       guide  = m_baseColor;
		guide.a            = 110;
		line(origin - axis * extent, origin + axis * extent, guide);
		line(origin, context.m_selectedNode3D->GetWorldPosition(), kActiveColor);
		m_dragLabel->SetText(Stringf("%+.3f", value));
		break;
	}
	case GizmoKind::Plane:
	{
		Vector3 const current = context.m_selectedNode3D->GetWorldPosition();
		Vector3 const delta   = current - origin;
		Vector3 const u       = context.m_basis.TransformDirection3D(GetPlaneU());
		Vector3 const v       = context.m_basis.TransformDirection3D(GetPlaneV());
		Color const colors[]  = { Color(255, 70, 105, 110), Color(155, 225, 20, 110), Color(55, 160, 255, 110) };
		char const* names[]   = { "X", "Y", "Z" };
		int const   uIndex    = m_axis == GizmoAxis::X ? 1 : 0;
		int const   vIndex    = m_axis == GizmoAxis::Z ? 1 : 2;
		float const extent    = scale * 100.f;
		line(current - u * extent, current + u * extent, colors[uIndex]);
		line(current - v * extent, current + v * extent, colors[vIndex]);
		m_dragLabel->SetText(Stringf("%s %+.3f / %s %+.3f",
			names[uIndex], DotProduct3D(delta, u), names[vIndex], DotProduct3D(delta, v)));
		break;
	}
	}

	m_dragVisual->SetWorldTransform(Matrix4x4::Identity);
	m_dragVisual->UploadGeometry(vertices);
	m_dragVisual->SetVisible(!vertices.empty());
	m_dragLabel->SetFontColor(m_kind == GizmoKind::Plane ? GetDrawColor() : kActiveColor);
	Vector2 const size = m_dragLabel->GetCombinedMinimumSize();
	m_dragLabel->UpdateSize(size);
	m_dragLabel->SetPosition(Vector2(
		std::clamp(context.m_clientPos.x + 18.f, 8.f, std::max(8.f, view.m_clientDimensions.x - size.x - 8.f)),
		std::clamp(context.m_clientPos.y + 18.f, 8.f, std::max(8.f, view.m_clientDimensions.y - size.y - 8.f))));
	m_dragLabel->SetVisible(true);
}

GizmoAxisArrow::GizmoAxisArrow(GizmoAxis axis, Color const& color, GizmoKind kind) : GizmoComponent(axis, color)
{
	Vector3 const axisDir = GetAxisWorld();
	Vector3 const start   = Vector3::Zero;
	Vector3 const end     = axisDir * kGizmoAxisLength;
	m_kind                = kind;
	if (kind == GizmoKind::UniformScale)
	{
		AddVertsForAABB3D(m_verts, AABB3(Vector3(-0.08f, -0.08f, -0.08f), Vector3(0.08f, 0.08f, 0.08f)), Color::White);
	}
	else
	{
		AddVertsForArrow3D(m_verts, start, end, kGizmoArrowRadius, Color::White);
		if (kind == GizmoKind::Scale)
		{
			Vector3 const half(0.08f, 0.08f, 0.08f);
			AddVertsForAABB3D(m_verts, AABB3(end - half, end + half), Color::White);
		}
	}

	m_virtualCenter = kind == GizmoKind::UniformScale ? Vector3::Zero : axisDir * (kGizmoAxisLength * 0.5f);
}

MathRaycastResult3D GizmoAxisArrow::Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	if (m_kind == GizmoKind::UniformScale)
	{
		return RaycastVsSphere3D(
			raycastInfo.m_startPos,
			raycastInfo.m_forwardNormal,
			raycastInfo.m_maxLength,
			context.m_originWorld,
			context.m_scale * 0.14f);
	}

	Vector3 const axisWorld     = GetContextAxis(context);
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
	m_startScale    = context.m_selectedNode3D->GetLocalScale();
	m_startWorld    = context.m_selectedNode3D->GetWorldTransform();
	m_startClient   = context.m_clientPos;
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

	if (m_kind == GizmoKind::UniformScale)
	{
		Vector2 const delta  = context.m_clientPos - m_startClient;
		float const   factor = std::exp(std::clamp((delta.x - delta.y) / 120.f, -8.f, 8.f));
		Vector3       scale  = m_startScale * factor;
		scale.x              = std::copysign(std::max(0.0001f, std::abs(scale.x)), m_startScale.x);
		scale.y              = std::copysign(std::max(0.0001f, std::abs(scale.y)), m_startScale.y);
		scale.z              = std::copysign(std::max(0.0001f, std::abs(scale.z)), m_startScale.z);
		context.m_selectedNode3D->SetLocalScale(scale);
		UpdateDragFeedback(context, factor);
		return;
	}

	Vector3 const axis           = GetContextAxis(context);
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
	if (m_kind == GizmoKind::Scale)
	{
		float const factor = std::max(0.0001f, 1.f + axisDelta / std::max(0.0001f, context.m_scale * kGizmoAxisLength));
		Vector3     scale  = m_startScale;
		if (context.m_local)
		{
			switch (m_axis)
			{
			case GizmoAxis::X:
				scale.x *= factor;
				break;
			case GizmoAxis::Y:
				scale.y *= factor;
				break;
			case GizmoAxis::Z:
				scale.z *= factor;
				break;
			}
		}
		else
		{
			Node3D*         parent  = dynamic_cast<Node3D*>(context.m_selectedNode3D->GetParent());
			Matrix4x4 const inverse = parent != nullptr ? parent->GetWorldInverseTransform() : Matrix4x4::Identity;
			auto            length  = [&](Vector3 basis, float original)
			{
				basis += axis * DotProduct3D(basis, axis) * (factor - 1.f);
				return std::copysign(inverse.TransformDirection3D(basis).GetLength(), original);
			};
			scale = Vector3(
				length(m_startWorld.GetIBasis3D(), m_startScale.x),
				length(m_startWorld.GetJBasis3D(), m_startScale.y),
				length(m_startWorld.GetKBasis3D(), m_startScale.z));
		}

		scale.x = std::copysign(std::max(0.0001f, std::abs(scale.x)), m_startScale.x);
		scale.y = std::copysign(std::max(0.0001f, std::abs(scale.y)), m_startScale.y);
		scale.z = std::copysign(std::max(0.0001f, std::abs(scale.z)), m_startScale.z);
		context.m_selectedNode3D->SetLocalScale(scale);
		UpdateDragFeedback(context, factor);
		return;
	}

	context.m_selectedNode3D->SetWorldPosition(m_startPosition + translation);
	UpdateDragFeedback(context, axisDelta);
}

GizmoPlaneSquare::GizmoPlaneSquare(GizmoAxis axis, Color const& color) : GizmoComponent(axis, color)
{
	m_kind               = GizmoKind::Plane;
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
	Vector3 const planeU      = context.m_basis.TransformDirection3D(GetPlaneU() * context.m_planeSigns);
	Vector3 const planeV      = context.m_basis.TransformDirection3D(GetPlaneV() * context.m_planeSigns);
	Vector3 const planeCenter = context.m_originWorld + (planeU + planeV) * (kGizmoPlaneOffset * context.m_scale);
	float const   planeHalf   = kGizmoPlanePickSize * context.m_scale * 0.5f;
	MathRaycastResult3D const result = RaycastVsPlane3D(
		raycastInfo.m_startPos,
		raycastInfo.m_forwardNormal,
		raycastInfo.m_maxLength,
		planeCenter,
		GetContextAxis(context));
	if (result.m_didImpact)
	{
		Vector3 const offset = result.m_impactPos - planeCenter;
		if (std::abs(DotProduct3D(offset, planeU)) > planeHalf || std::abs(DotProduct3D(offset, planeV)) > planeHalf)
		{
			return MathRaycastResult3D(raycastInfo.m_startPos, raycastInfo.m_forwardNormal, raycastInfo.m_maxLength);
		}
	}

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

	Vector3 const             planeNormal = GetContextAxis(context);
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
	UpdateDragFeedback(context, translation.GetLength());
}

GizmoRotationArc::GizmoRotationArc(GizmoAxis axis, Color const& color) : GizmoComponent(axis, color)
{
	m_kind          = GizmoKind::Rotate;
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

GizmoRotationArc::~GizmoRotationArc() = default;

void GizmoRotationArc::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Process:
	{
		if (EditorCamera::Get() == nullptr)
		{
			return;
		}

		Vector3 const local  = GetWorldInverseTransform().TransformPosition3D(EditorCamera::Get()->GetWorldPosition());
		Vector3 const normal = GetAxisWorld();
		m_virtualCenter      = (local - normal * DotProduct3D(local, normal)).GetNormalized() * kGizmoRotationRadius;
		break;
	}
	}
}

MathRaycastResult3D GizmoRotationArc::Raycast(GizmoContext const& context, MathRaycastQuery3D const& raycastInfo) const
{
	Vector3 const       worldNormal = GetContextAxis(context);
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

	m_startLocalRotation = context.m_selectedNode3D->GetLocalOrientation().GetAsMatrix_IFwd_JLeft_KUp();
	m_dragOrigin         = context.m_originWorld;
	m_startVectorWorld   = (hitPos - context.m_originWorld).GetNormalized();
	m_currentDegrees     = 0.f;
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
		GetContextAxis(context));
	if (!hit.m_didImpact)
	{
		return;
	}

	Vector3       currentVector = (hit.m_impactPos - m_dragOrigin).GetNormalized();
	Vector3 const cross         = CrossProduct3D(m_startVectorWorld, currentVector);
	float const   sinVal        = DotProduct3D(cross, GetContextAxis(context));
	float const   cosVal        = DotProduct3D(m_startVectorWorld, currentVector);
	m_currentDegrees            = ConvertRadiansToDegrees(atan2f(sinVal, cosVal));

	Node3D* parent = dynamic_cast<Node3D*>(context.m_selectedNode3D->GetParent());
	Vector3 axis =
		context.m_local ? m_startLocalRotation.TransformDirection3D(GetAxisWorld()) : GetContextAxis(context);
	if (!context.m_local && parent != nullptr)
	{
		axis = parent->GetWorldInverseTransform().TransformDirection3D(axis).GetNormalized();
	}

	Matrix4x4 rotation = AxisRotation(axis.GetNormalized(), m_currentDegrees);
	rotation.Append(m_startLocalRotation);
	rotation.Orthonormalize_XFwd_YLeft_ZUp2();
	EulerAngles orientation;
	orientation.SetFromMatrix_IFwd_JLeft_KUp(rotation);
	context.m_selectedNode3D->SetLocalOrientation(orientation);
	UpdateDragFeedback(context, m_currentDegrees);
}

void GizmoRotationArc::OnEndDrag(GizmoContext const& context)
{
	GizmoComponent::OnEndDrag(context);
	m_currentDegrees = 0.f;
}

bool GizmoRotationArc::IsRotationGizmo() const
{
	return true;
}
