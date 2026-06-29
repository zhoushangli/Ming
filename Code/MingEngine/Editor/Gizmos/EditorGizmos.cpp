#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"

#include "MingEngine/Editor/EditorController.hpp"
#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"
#include "MingEngine/Editor/Gizmos/ViewportAxisIndicator.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"

using namespace Math;

namespace
{
float constexpr kEditorGizmoRaycastLength = 10000.f;
} // namespace

EditorGizmos::EditorGizmos()
{
	SetReady(true);
	SetProcess(true);

	m_worldGrid = new EditorWorldGrid3D();
	m_worldGrid->SetName("WorldGrid");
	m_worldGrid->SetSerializable(false);
	AddNode(m_worldGrid);

	m_worldAxis = new EditorWorldAxis3D();
	m_worldAxis->SetName("WorldAxis");
	m_worldAxis->SetSerializable(false);
	AddNode(m_worldAxis);

	m_transformGizmo = new TransformGizmo3D();
	m_transformGizmo->SetName("TransformGizmo3D");
	m_transformGizmo->SetSerializable(false);
	AddNode(m_transformGizmo);

	m_axisIndicator = new ViewportAxisIndicator();
	m_axisIndicator->SetName("ViewportAxisIndicator");
	m_axisIndicator->SetSerializable(false);
	AddNode(m_axisIndicator);
}

EditorGizmos::~EditorGizmos() {}

void EditorGizmos::OnMouseMove(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx    = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	float const        aspect = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext      camCtx = camera.GetCameraContext(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);
	m_transformGizmo->UpdateHover(ctx, ray);
}

bool EditorGizmos::OnBeginDrag(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx    = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	float const        aspect = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext      camCtx = camera.GetCameraContext(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);
	return m_transformGizmo->BeginDrag(ctx, ray);
}

void EditorGizmos::OnDrag(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx    = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	float const        aspect = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext      camCtx = camera.GetCameraContext(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);
	m_transformGizmo->OnDrag(ctx, ray);
}

void EditorGizmos::OnEndDrag()
{
	if (m_transformGizmo->IsDragging())
	{
		Camera3D* camera = EditorController::Get() ? EditorController::Get()->GetCamera() : nullptr;
		if (camera != nullptr)
		{
			Vec2 const         cursorPos = g_engine->m_inputSystem->GetCursorClientPosition();
			GizmoContext const ctx       = BuildGizmoContext(GetSceneTree(), *camera, cursorPos);
			m_transformGizmo->EndDrag(ctx);
		}
	}
}

bool EditorGizmos::IsDragging() const { return m_transformGizmo != nullptr && m_transformGizmo->IsDragging(); }

NodeHandle EditorGizmos::Raycast(Camera3D const& camera, Vec2 screenPos) const
{
	GizmoContext const ctx    = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	float const        aspect = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext      camCtx = camera.GetCameraContext(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);

	GizmoRaycastResult const result = m_transformGizmo->Raycast(ctx, ray);
	if (result.m_didImpact && result.m_component != nullptr)
	{
		return result.m_component->GetHandle();
	}

	return NodeHandle::Invalid;
}
