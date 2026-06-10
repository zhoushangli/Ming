#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"

#include "MingEngine/Editor/EditorController.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"
#include "MingEngine/Editor/Gizmos/ViewportAxisIndicator.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"

namespace
{
float constexpr kEditorGizmoRaycastLength = 10000.f;
} // namespace

void EditorGizmos::OnReady()
{
	Node::OnReady();
	SetWorldGridVisible(true);
	SetWorldAxisVisible(false);
}

EditorGizmos::EditorGizmos()
{
	SetReady(true);
	SetProcess(true);

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

void EditorGizmos::OnProcess([[maybe_unused]] float deltaSeconds)
{
}

void EditorGizmos::SetWorldGridVisible(bool visible)
{
	SceneTree* sceneTree = GetSceneTree();
	Node* worldGrid      = sceneTree != nullptr ? sceneTree->ResolveNode(m_worldGridHandle) : nullptr;

	if (visible)
	{
		if (worldGrid == nullptr)
		{
			EditorWorldGrid3D* newWorldGrid = new EditorWorldGrid3D();
			newWorldGrid->SetName("WorldGrid");
			newWorldGrid->SetSerializable(false);
			AddNode(newWorldGrid);
			m_worldGridHandle = newWorldGrid->GetHandle();
		}
		return;
	}

	if (worldGrid != nullptr)
	{
		worldGrid->DeleteNode();
	}
	m_worldGridHandle = NodeHandle::Invalid;
}

void EditorGizmos::SetWorldAxisVisible(bool visible)
{
	SceneTree* sceneTree = GetSceneTree();
	Node* worldAxis      = sceneTree != nullptr ? sceneTree->ResolveNode(m_worldAxisHandle) : nullptr;

	if (visible)
	{
		if (worldAxis == nullptr)
		{
			EditorWorldAxis3D* newWorldAxis = new EditorWorldAxis3D();
			newWorldAxis->SetName("WorldAxis");
			newWorldAxis->SetSerializable(false);
			AddNode(newWorldAxis);
			m_worldAxisHandle = newWorldAxis->GetHandle();
		}
		return;
	}

	if (worldAxis != nullptr)
	{
		worldAxis->DeleteNode();
	}
	m_worldAxisHandle = NodeHandle::Invalid;
}

GizmoContext EditorGizmos::BuildGizmoContext(Camera3D const& camera, Vec2 screenPos) const
{
	GizmoContext context;
	context.m_sceneTree = GetSceneTree();
	context.m_camera    = &camera;
	context.m_clientPos = screenPos;
	context.m_clientDimensions =
		g_engine && g_engine->m_window ? (Vec2)g_engine->m_window->GetClientDimensions() : Vec2::One;

	EditorNode* editorNode = EditorNode::Get();
	if (editorNode != nullptr)
	{
		NodeHandle selectedHandle = editorNode->GetSelection().GetSelected();
		context.m_selectedNode    = selectedHandle;
		context.m_selectedNode3D  = dynamic_cast<Node3D*>(
			context.m_sceneTree != nullptr ? context.m_sceneTree->ResolveNode(selectedHandle) : nullptr);

		if (context.m_selectedNode3D != nullptr)
		{
			context.m_originWorld = context.m_selectedNode3D->GetWorldPosition();
		}
	}

	// Scale to keep gizmo constant screen size
	if (context.m_selectedNode3D != nullptr)
	{
		Vec3 const cameraPos   = context.m_selectedNode3D->GetWorldPosition();
		float const cameraDist = (context.m_originWorld - camera.GetWorldPosition()).GetLength();
		context.m_scale        = Max(0.1f, cameraDist) * 0.15f;
	}
	else
	{
		context.m_scale = 1.f;
	}

	return context;
}

void EditorGizmos::OnMouseMove(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(camera, screenPos);
	float const aspect     = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext camCtx   = camera.GetCamera(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);
	m_transformGizmo->UpdateHover(ctx, ray);
}

bool EditorGizmos::OnBeginDrag(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(camera, screenPos);
	float const aspect     = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext camCtx   = camera.GetCamera(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);
	return m_transformGizmo->TryBeginDrag(ctx, ray);
}

void EditorGizmos::OnDrag(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(camera, screenPos);
	float const aspect     = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext camCtx   = camera.GetCamera(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);
	m_transformGizmo->UpdateDrag(ctx, ray);
}

void EditorGizmos::OnEndDrag()
{
	if (m_transformGizmo->IsDragging())
	{
		Camera3D* camera = EditorController::Get() ? EditorController::Get()->GetCamera() : nullptr;
		if (camera != nullptr)
		{
			Vec2 const cursorPos   = g_engine->m_input->GetCursorClientPosition();
			GizmoContext const ctx = BuildGizmoContext(*camera, cursorPos);
			m_transformGizmo->EndDrag(ctx);
		}
	}
}

bool EditorGizmos::IsDragging() const { return m_transformGizmo != nullptr && m_transformGizmo->IsDragging(); }

NodeHandle EditorGizmos::Raycast(Camera3D const& camera, Vec2 screenPos) const
{
	GizmoContext const ctx = BuildGizmoContext(camera, screenPos);
	float const aspect     = ctx.m_clientDimensions.x / Max(ctx.m_clientDimensions.y, 1.f);
	CameraContext camCtx   = camera.GetCamera(aspect);
	RaycastInfo ray = BuildRaycastFromMouse(camCtx, screenPos, ctx.m_clientDimensions, kEditorGizmoRaycastLength);

	GizmoRaycastResult const result = m_transformGizmo->Raycast(ctx, ray);
	if (result.m_didImpact && result.m_component != nullptr)
	{
		return result.m_component->GetHandle();
	}

	return NodeHandle::Invalid;
}
