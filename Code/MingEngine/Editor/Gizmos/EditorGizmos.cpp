#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"

#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"
#include "MingEngine/Editor/Gizmos/ViewportAxisIndicator.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/CameraContext.hpp"

using namespace Math;

namespace
{
float constexpr kEditorGizmoRaycastLength = 10000.f;
float constexpr kWorldAxisExtent          = 100.f;

Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);

EditorWorldAxis3D* CreateWorldAxis(
	Node* parent, char const* name, Vec3 const& axisStart, Vec3 const& axisEnd, Rgba8 const& color)
{
	EditorWorldAxis3D* axis = new EditorWorldAxis3D(axisStart, axisEnd, color);
	axis->SetName(name);
	axis->SetSerializable(false);
	parent->AddNode(axis);
	return axis;
}
} // namespace

EditorGizmos::EditorGizmos()
{
	SetReady(true);
	SetProcess(true);

	m_worldGrid = new EditorWorldGrid3D();
	m_worldGrid->SetName("WorldGrid");
	m_worldGrid->SetSerializable(false);
	AddNode(m_worldGrid);

	m_worldAxises[0] =
		CreateWorldAxis(this, "WorldAxisXPositive", Vec3::Zero, Vec3(kWorldAxisExtent, 0.f, 0.f), kAxisXColor);
	m_worldAxises[1] =
		CreateWorldAxis(this, "WorldAxisXNegative", Vec3::Zero, Vec3(-kWorldAxisExtent, 0.f, 0.f), kAxisXColor);
	m_worldAxises[2] =
		CreateWorldAxis(this, "WorldAxisYPositive", Vec3::Zero, Vec3(0.f, kWorldAxisExtent, 0.f), kAxisYColor);
	m_worldAxises[3] =
		CreateWorldAxis(this, "WorldAxisYNegative", Vec3::Zero, Vec3(0.f, -kWorldAxisExtent, 0.f), kAxisYColor);
	m_worldAxises[4] =
		CreateWorldAxis(this, "WorldAxisZPositive", Vec3::Zero, Vec3(0.f, 0.f, kWorldAxisExtent), kAxisZColor);
	m_worldAxises[5] =
		CreateWorldAxis(this, "WorldAxisZNegative", Vec3::Zero, Vec3(0.f, 0.f, -kWorldAxisExtent), kAxisZColor);

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
	GizmoContext const ctx = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	m_transformGizmo->UpdateHover(ctx);
}

bool EditorGizmos::BeginDragHovered(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	return m_transformGizmo->BeginDragHovered(ctx);
}

void EditorGizmos::OnDrag(Camera3D const& camera, Vec2 screenPos)
{
	GizmoContext const ctx = BuildGizmoContext(GetSceneTree(), camera, screenPos);
	m_transformGizmo->OnDrag(ctx);
}

void EditorGizmos::OnEndDrag()
{
	if (m_transformGizmo->IsDragging())
	{
		Camera3D* camera = EditorCamera::Get() ? EditorCamera::Get()->GetCamera() : nullptr;
		if (camera != nullptr)
		{
			Vec2 const         cursorPos = Vec2(g_engine->m_inputSystem->GetCursorClientPosition());
			GizmoContext const ctx       = BuildGizmoContext(GetSceneTree(), *camera, cursorPos);
			m_transformGizmo->EndDrag(ctx);
		}
	}
}

bool EditorGizmos::IsHovered() const { return m_transformGizmo != nullptr && m_transformGizmo->IsHovered(); }

bool EditorGizmos::IsDragging() const { return m_transformGizmo != nullptr && m_transformGizmo->IsDragging(); }
