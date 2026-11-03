#include "MingEngine/Editor/Gizmos/TransformGizmo3D.hpp"

#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

namespace
{
Color const     kAxisXColor(255, 70, 105, 255);
Color const     kAxisYColor(155, 225, 20, 255);
Color const     kAxisZColor(55, 160, 255, 255);
constexpr float kRaycastMaxLength = 10000.f;

} // namespace

TransformGizmo3D::TransformGizmo3D()
{
	SetReady(true);
	SetProcess(true);

	GizmoAxisArrow*   arrowX = MemNew<GizmoAxisArrow>(GizmoAxis::X, kAxisXColor);
	GizmoAxisArrow*   arrowY = MemNew<GizmoAxisArrow>(GizmoAxis::Y, kAxisYColor);
	GizmoAxisArrow*   arrowZ = MemNew<GizmoAxisArrow>(GizmoAxis::Z, kAxisZColor);
	GizmoPlaneSquare* planeX = MemNew<GizmoPlaneSquare>(GizmoAxis::X, kAxisXColor);
	GizmoPlaneSquare* planeY = MemNew<GizmoPlaneSquare>(GizmoAxis::Y, kAxisYColor);
	GizmoPlaneSquare* planeZ = MemNew<GizmoPlaneSquare>(GizmoAxis::Z, kAxisZColor);
	GizmoRotationArc* arcX   = MemNew<GizmoRotationArc>(GizmoAxis::X, kAxisXColor);
	GizmoRotationArc* arcY   = MemNew<GizmoRotationArc>(GizmoAxis::Y, kAxisYColor);
	GizmoRotationArc* arcZ   = MemNew<GizmoRotationArc>(GizmoAxis::Z, kAxisZColor);

	arrowX->SetSerializable(false);
	arrowY->SetSerializable(false);
	arrowZ->SetSerializable(false);
	planeX->SetSerializable(false);
	planeY->SetSerializable(false);
	planeZ->SetSerializable(false);
	arcX->SetSerializable(false);
	arcY->SetSerializable(false);
	arcZ->SetSerializable(false);

	AddNode(arrowX);
	AddNode(arrowY);
	AddNode(arrowZ);
	AddNode(planeX);
	AddNode(planeY);
	AddNode(planeZ);
	AddNode(arcX);
	AddNode(arcY);
	AddNode(arcZ);

	m_components.push_back(arrowX);
	m_components.push_back(arrowY);
	m_components.push_back(arrowZ);
	m_components.push_back(planeX);
	m_components.push_back(planeY);
	m_components.push_back(planeZ);
	m_components.push_back(arcX);
	m_components.push_back(arcY);
	m_components.push_back(arcZ);
	Color const colors[] = { kAxisXColor, kAxisYColor, kAxisZColor, Color::White };
	for (int i = 0; i < 4; ++i)
	{
		auto* handle = MemNew<GizmoAxisArrow>(
			static_cast<GizmoAxis>(i % 3),
			colors[i],
			i == 3 ? GizmoKind::UniformScale : GizmoKind::Scale);
		handle->SetSerializable(false);
		AddNode(handle);
		m_components.push_back(handle);
	}
}

TransformGizmo3D::~TransformGizmo3D()
{
	m_components.clear();
}

void TransformGizmo3D::UpdateHover(GizmoContext const& context)
{
	if (m_activeComponent != nullptr)
	{
		return;
	}

	m_hoveredHitPos              = context.m_originWorld;
	GizmoComponent* hitComponent = HitTest(context, m_hoveredHitPos);

	if (m_hoveredComponent != hitComponent)
	{
		if (m_hoveredComponent != nullptr)
		{
			m_hoveredComponent->SetHovered(false);
		}

		m_hoveredComponent = hitComponent;
		if (m_hoveredComponent != nullptr)
		{
			m_hoveredComponent->SetHovered(true);
		}
	}
}

bool TransformGizmo3D::BeginDragHovered(GizmoContext const& context)
{
	UpdateHover(context);
	if (m_hoveredComponent == nullptr || context.m_selectedNode3D == nullptr)
	{
		return false;
	}

	if (context.m_camera == nullptr)
	{
		return false;
	}

	m_dragStartRaycastInfo =
		context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, kRaycastMaxLength);
	m_hoveredComponent->SetHovered(true);
	m_activeComponent = m_hoveredComponent;
	m_dragContext     = context;
	Node* scene       = context.m_sceneTree != nullptr ? context.m_sceneTree->GetScene() : nullptr;
	m_dragSceneID     = scene != nullptr ? scene->GetObjectID() : ObjectID::Invalid;
	if (context.m_selectedNode3D != nullptr)
	{
		m_draggedNodeID        = context.m_selectedNode3D->GetObjectID();
		m_dragStartPosition    = context.m_selectedNode3D->GetLocalPosition();
		m_dragStartOrientation = context.m_selectedNode3D->GetLocalOrientation();
		m_dragStartScale       = context.m_selectedNode3D->GetLocalScale();
	}

	m_activeComponent->OnBeginDrag(context, m_hoveredHitPos);
	return true;
}

void TransformGizmo3D::OnDrag(GizmoContext const& context)
{
	Node3D* target = ObjectDatabase::GetInstance<Node3D>(m_draggedNodeID);
	if (m_activeComponent == nullptr)
	{
		return;
	}

	if (target == nullptr || target->IsQueuedForDeletion() || target->GetSceneTree() != context.m_sceneTree
		|| context.m_selectedNodeID != m_draggedNodeID)
	{
		CancelDrag();
		return;
	}

	GizmoContext frozen       = m_dragContext;
	frozen.m_selectedNode3D   = target;
	frozen.m_clientPos        = context.m_clientPos;
	frozen.m_clientDimensions = context.m_clientDimensions;
	MathRaycastQuery3D const ray =
		frozen.m_camera->BuildRaycastFromMouse(frozen.m_clientPos, frozen.m_clientDimensions, kRaycastMaxLength);
	m_activeComponent->OnDrag(frozen, m_dragStartRaycastInfo, ray);
}

void TransformGizmo3D::EndDrag(GizmoContext const& context)
{
	if (m_activeComponent == nullptr)
	{
		return;
	}

	Node3D* target = ObjectDatabase::GetInstance<Node3D>(m_draggedNodeID);
	Node*   scene  = context.m_sceneTree != nullptr ? context.m_sceneTree->GetScene() : nullptr;
	if (target == nullptr || target->IsQueuedForDeletion() || target->GetSceneTree() != context.m_sceneTree
		|| scene == nullptr || scene->GetObjectID() != m_dragSceneID)
	{
		CancelDrag();
		return;
	}

	m_activeComponent->OnEndDrag(context);
	bool const  changed = target->GetLocalPosition() != m_dragStartPosition
						  || target->GetLocalOrientation() != m_dragStartOrientation
						  || target->GetLocalScale() != m_dragStartScale;
	EditorNode* editor  = EditorNode::Get();
	if (changed && editor != nullptr)
	{
		EditorUndoRedo& undo = editor->GetUndoRedo();
		if (undo.CreateAction("Transform Node", m_dragSceneID))
		{
			undo.AddDoMethod(target, "SetLocalPosition", { target->GetLocalPosition() });
			undo.AddDoMethod(target, "SetLocalOrientation", { target->GetLocalOrientation() });
			undo.AddDoMethod(target, "SetLocalScale", { target->GetLocalScale() });
			undo.AddDoMethod(editor, "MarkSceneDirty");
			undo.AddUndoMethod(target, "SetLocalPosition", { m_dragStartPosition });
			undo.AddUndoMethod(target, "SetLocalOrientation", { m_dragStartOrientation });
			undo.AddUndoMethod(target, "SetLocalScale", { m_dragStartScale });
			undo.AddUndoMethod(editor, "MarkSceneDirty");
			undo.CommitAction(false);
		}

		editor->MarkSceneDirty();
	}

	m_activeComponent = nullptr;
	m_draggedNodeID   = ObjectID::Invalid;
	ClearHover();
}

void TransformGizmo3D::CancelDrag()
{
	if (m_activeComponent != nullptr)
	{
		Node3D* target = ObjectDatabase::GetInstance<Node3D>(m_draggedNodeID);
		if (target != nullptr && !target->IsQueuedForDeletion() && target->GetSceneTree() == m_dragContext.m_sceneTree)
		{
			target->SetLocalPosition(m_dragStartPosition);
			target->SetLocalOrientation(m_dragStartOrientation);
			target->SetLocalScale(m_dragStartScale);
		}

		m_activeComponent->OnEndDrag(m_dragContext);
	}

	m_activeComponent = nullptr;
	m_draggedNodeID   = ObjectID::Invalid;
	ClearHover();
}

void TransformGizmo3D::ClearHover()
{
	if (m_hoveredComponent != nullptr)
	{
		m_hoveredComponent->SetHovered(false);
		m_hoveredComponent = nullptr;
	}
}

void TransformGizmo3D::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Process:
	{
		EditorCamera* editorCamera = EditorCamera::Get();
		Camera3D*     camera       = editorCamera != nullptr ? editorCamera->GetCamera() : nullptr;
		if (camera == nullptr)
		{
			for (GizmoComponent* component : m_components)
			{
				component->SetVisible(false);
			}

			return;
		}

		GizmoContext const context      = BuildGizmoContext(GetSceneTree(), *camera);
		bool const         hasSelection = context.m_selectedNode3D != nullptr;
		if (m_activeComponent != nullptr && (!hasSelection || context.m_selectedNodeID != m_draggedNodeID))
		{
			CancelDrag();
		}

		if (!hasSelection)
		{
			ClearHover();
		}

		bool const rotationActive = m_activeComponent != nullptr && m_activeComponent->IsRotationGizmo();

		for (GizmoComponent* component : m_components)
		{
			bool const allowed   = component->IsEnabledForTool(context.m_tool);
			bool const isVisible = hasSelection && allowed && (!rotationActive || component == m_activeComponent);
			component->SetVisible(isVisible);
			if (isVisible)
			{
				Matrix4x4 const& basis = IsDragging() ? m_dragContext.m_basis : context.m_basis;
				EulerAngles      orientation;
				orientation.SetFromMatrix_IFwd_JLeft_KUp(basis);
				float const scale = IsDragging() ? m_dragContext.m_scale : context.m_scale;
				component->SetWorldPosition(context.m_originWorld);
				component->SetWorldOrientation(orientation);
				component->SetWorldScale(Vector3(scale, scale, scale));
			}
		}

		Vector3 const cameraPos = camera->GetWorldPosition();
		std::stable_sort(
			m_components.begin(),
			m_components.end(),
			[&](GizmoComponent* a, GizmoComponent* b)
			{
				float const distA = (a->GetWorldVirtualCenter() - cameraPos).GetLengthSquared();
				float const distB = (b->GetWorldVirtualCenter() - cameraPos).GetLengthSquared();
				return distA > distB;
			});

		int constexpr kGizmoPriorityBase = 1000;
		for (size_t rank = 0; rank < m_components.size(); ++rank)
		{
			m_components[rank]->SetRenderPriority(kGizmoPriorityBase + static_cast<int>(rank));
		}

		break;
	}
	}
}

bool TransformGizmo3D::IsHovered() const
{
	return m_hoveredComponent != nullptr;
}

bool TransformGizmo3D::IsDragging() const
{
	return m_activeComponent != nullptr;
}

GizmoComponent* TransformGizmo3D::HitTest(GizmoContext const& context, Vector3& outHitPos) const
{
	if (context.m_selectedNode3D == nullptr || context.m_camera == nullptr)
	{
		return nullptr;
	}

	MathRaycastQuery3D const raycastInfo =
		context.m_camera->BuildRaycastFromMouse(context.m_clientPos, context.m_clientDimensions, kRaycastMaxLength);
	GizmoComponent* closestComponent = nullptr;
	float           closestDistance  = raycastInfo.m_maxLength;
	for (GizmoComponent* component : m_components)
	{
		if (component == nullptr || !component->IsEnabledForTool(context.m_tool))
		{
			continue;
		}

		MathRaycastResult3D const hit = component->Raycast(context, raycastInfo);
		if (hit.m_didImpact && hit.m_impactDist < closestDistance)
		{
			closestComponent = component;
			closestDistance  = hit.m_impactDist;
			outHitPos        = hit.m_impactPos;
		}
	}

	return closestComponent;
}
