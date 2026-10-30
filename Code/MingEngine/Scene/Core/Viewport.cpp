#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"

#include <algorithm>

Viewport::Viewport()
{
	IntVec2 defaultResolution =
		g_engine->m_windowSystem != nullptr ? g_engine->m_windowSystem->GetClientDimensions() : IntVec2(1280, 720);

	m_viewportRID    = g_engine->m_renderServer->ViewportCreate();
	m_canvasLayerRID = g_engine->m_renderServer->CanvasLayerCreate();
	g_engine->m_renderServer->CanvasLayerSetViewport(m_canvasLayerRID, m_viewportRID);
	m_outputResolution = defaultResolution;
	g_engine->m_renderServer->ViewportSetResolution(m_viewportRID, m_outputResolution);

	// Every Viewport owns one Scenario so scene nodes have a target to register instances into.
	m_scenarioRID = g_engine->m_renderServer->ScenarioCreate();
	g_engine->m_renderServer->ViewportSetScenario(m_viewportRID, m_scenarioRID);
}

Viewport::~Viewport()
{
	// Viewport GPU resources must be destroyed while Engine and Renderer still exist.
	g_engine->m_renderServer->ScenarioFree(m_scenarioRID);
	g_engine->m_renderServer->CanvasLayerFree(m_canvasLayerRID);
	g_engine->m_renderServer->ViewportFree(m_viewportRID);
}

void Viewport::OnNotification(int notification)
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr)
	{
		return;
	}

	switch (notification)
	{
	case Notification_EnterTree:
		server->ViewportSetActive(m_viewportRID, true);
		break;
	case Notification_ExitTree:
		// Leaving the tree only deactivates: the RID and its size survive for re-entering.
		server->ViewportSetActive(m_viewportRID, false);
		break;
	}
}

void Viewport::AddCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	m_cameras.insert(camera);
}

void Viewport::RemoveCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	m_cameras.erase(camera);
	if (m_currentCamera == camera)
	{
		SetCurrentCamera(nullptr);
	}
}

void Viewport::SetCurrentCamera(Camera3D* camera)
{
	if (camera == m_currentCamera)
	{
		return;
	}

	m_currentCamera = camera;

	if (camera != nullptr)
	{
		g_engine->m_renderServer->ViewportSetCamera(m_viewportRID, camera->GetCameraRID());
	}
	else
	{
		g_engine->m_renderServer->ViewportSetCamera(m_viewportRID, RID::Invalid);
	}
}

Camera3D* Viewport::GetCurrentCamera() const
{
	return m_currentCamera;
}

void Viewport::ChangeToNextCamera()
{
	for (auto& it : m_cameras)
	{
		it->SetCurrent();
		return;
	}
}

IntVec2 Viewport::GetOutputResolution() const
{
	return m_outputResolution;
}

void Viewport::UpdateResolution(IntVec2 dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	SetResolution(dimensions);

	std::vector<ObjectID> const roots = m_gui.m_rootControls;
	for (ObjectID id : roots)
	{
		if (Control* rootControl = ResolveGuiControl(id, false))
		{
			rootControl->UpdateSize((Vector2)dimensions);
		}
	}
}

void Viewport::AddRootControl(Control* control)
{
	if (control == nullptr)
	{
		return;
	}

	ObjectID const id = control->GetObjectID();
	if (std::find(m_gui.m_rootControls.begin(), m_gui.m_rootControls.end(), id) != m_gui.m_rootControls.end())
	{
		return;
	}

	m_gui.m_rootControls.push_back(id);
	control->UpdateSize(Vector2(m_outputResolution));
}

void Viewport::RemoveRootControl(Control* control)
{
	ObjectID const id    = control->GetObjectID();
	auto&          roots = m_gui.m_rootControls;
	roots.erase(std::remove(roots.begin(), roots.end(), id), roots.end());
}

void Viewport::SetResolution(IntVec2 dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	if (m_outputResolution == dimensions)
	{
		return;
	}

	m_outputResolution = dimensions;
	g_engine->m_renderServer->ViewportSetResolution(m_viewportRID, dimensions);
}

Control* Viewport::ResolveGuiControl(ObjectID id, bool requireVisible) const
{
	Control* control = ObjectDatabase::GetInstance<Control>(id);
	if (control == nullptr || control->GetSceneTree() == nullptr || control->GetViewport() != this
		|| (requireVisible && !control->IsVisibleInHierarchy()))
	{
		return nullptr;
	}

	for (Node* node = control; node != nullptr; node = node->GetParent())
	{
		if (node->IsQueuedForDeletion())
		{
			return nullptr;
		}
	}

	return control;
}

Control* Viewport::FindControlInSubtree(Node* node, Vector2 const& position) const
{
	if (node->GetObjectID() == m_gui.m_dragPreview)
	{
		return nullptr;
	}

	if (node->GetViewport() != this || node->IsQueuedForDeletion() || dynamic_cast<Viewport*>(node) != nullptr)
	{
		return nullptr;
	}

	if (CanvasItem* item = dynamic_cast<CanvasItem*>(node); item != nullptr && !item->IsVisibleInHierarchy())
	{
		return nullptr;
	}

	auto const& children = node->GetChildren();
	if (auto* scroll = dynamic_cast<ScrollContainer*>(node))
	{
		Vector2 const local = position - scroll->GetGlobalPosition();
		Vector2 const page = scroll->GetPageSize();
		if (scroll->IsPointInside(local) && (local.x >= page.x || local.y >= page.y))
		{
			return scroll;
		}
	}
	if (Control* clip = dynamic_cast<Control*>(node);
		clip != nullptr && clip->IsClippingContents() && !clip->IsPointInside(position - clip->GetGlobalPosition()))
	{
		return nullptr;
	}

	for (auto it = children.rbegin(); it != children.rend(); ++it)
	{
		if (Control* hit = FindControlInSubtree(*it, position))
		{
			return hit;
		}
	}

	Control* control = dynamic_cast<Control*>(node);
	if (control != nullptr && control->GetMouseFilter() != Control::MouseFilter::Ignore
		&& control->IsPointInside(position - control->GetGlobalPosition()))
	{
		return control;
	}

	return nullptr;
}

Control* Viewport::FindControlAtPosition(Vector2 const& position) const
{
	if (Control* popup = GetGuiPopup())
	{
		Control* hit = FindControlInSubtree(popup, position);
		return hit != nullptr ? hit : popup;
	}

	if (position.x < 0.f || position.y < 0.f || position.x >= m_outputResolution.x
		|| position.y >= m_outputResolution.y)
	{
		return nullptr;
	}

	for (auto it = m_gui.m_rootControls.rbegin(); it != m_gui.m_rootControls.rend(); ++it)
	{
		if (Control* root = ResolveGuiControl(*it))
		{
			if (Control* hit = FindControlInSubtree(root, position))
			{
				return hit;
			}
		}
	}

	return nullptr;
}

void Viewport::SetGuiHovered(Control* control)
{
	ObjectID const next = control != nullptr ? control->GetObjectID() : ObjectID::Invalid;
	if (next == m_gui.m_hoveredControl)
	{
		return;
	}

	ObjectID const previous = m_gui.m_hoveredControl;
	m_gui.m_hoveredControl  = next;
	if (Control* old = ResolveGuiControl(previous, false))
	{
		old->Notification(Control::Notification_MouseExit);
	}

	if (m_gui.m_hoveredControl == next)
	{
		if (Control* current = ResolveGuiControl(next))
		{
			current->Notification(Control::Notification_MouseEnter);
		}
	}
}

void Viewport::SetGuiFocus(Control* control)
{
	if (control != nullptr
		&& (ResolveGuiControl(control->GetObjectID()) == nullptr || control->IsDisabled()
			|| control->GetFocusMode() == Control::FocusMode::None))
	{
		return;
	}

	ObjectID const next    = control != nullptr ? control->GetObjectID() : ObjectID::Invalid;
	uint64_t const version = ++m_gui.m_focusVersion;
	if (next == m_gui.m_focusedControl)
	{
		return;
	}

	ObjectID const previous   = m_gui.m_focusedControl;
	ObjectID const viewportID = GetObjectID();
	m_gui.m_focusedControl    = ObjectID::Invalid;
	if (Control* old = ResolveGuiControl(previous, false))
	{
		old->Notification(Control::Notification_FocusExit);
	}

	Viewport* viewport = ObjectDatabase::GetInstance<Viewport>(viewportID);
	if (viewport == nullptr || viewport->m_gui.m_focusVersion != version)
	{
		return;
	}

	if (Control* current = viewport->ResolveGuiControl(next);
		current != nullptr && !current->IsDisabled() && current->GetFocusMode() != Control::FocusMode::None)
	{
		viewport->m_gui.m_focusedControl = next;
		current->Notification(Control::Notification_FocusEnter);
		viewport = ObjectDatabase::GetInstance<Viewport>(viewportID);
		if (viewport == nullptr || viewport->m_gui.m_focusVersion != version)
		{
			return;
		}

		current = viewport->ResolveGuiControl(next);
		if (current == nullptr)
		{
			return;
		}

		for (Node* parent = current->GetParent(); parent != nullptr; parent = parent->GetParent())
		{
			if (auto* scroll = dynamic_cast<ScrollContainer*>(parent))
			{
				scroll->EnsureControlVisible(current);
			}
		}
	}
}

void Viewport::SetGuiPopup(Control* control)
{
	if (control != nullptr)
	{
		CancelGuiDrag();
	}

	m_gui.m_popupControl = control != nullptr ? control->GetObjectID() : ObjectID::Invalid;
}

bool Viewport::HasGuiFocus(Control const* control) const
{
	return control != nullptr && m_gui.m_focusedControl == control->GetObjectID();
}

void Viewport::CancelGuiInteraction(Node* subtree, bool clearFocus, bool cancelDrag)
{
	ObjectID const viewportID = GetObjectID();
	auto           belongs    = [subtree](ObjectID id)
	{
		for (Node* node = ObjectDatabase::GetInstance<Node>(id); node != nullptr; node = node->GetParent())
		{
			if (node == subtree)
			{
				return true;
			}
		}

		return false;
	};
	if (cancelDrag && (subtree == this || belongs(m_gui.m_dragSource)))
	{
		CancelGuiDrag();
	}

	ObjectID const capture = belongs(m_gui.m_mouseCaptureControl) ? m_gui.m_mouseCaptureControl : ObjectID::Invalid;
	ObjectID const hover   = belongs(m_gui.m_hoveredControl) ? m_gui.m_hoveredControl : ObjectID::Invalid;
	ObjectID const focus   = belongs(m_gui.m_focusedControl) ? m_gui.m_focusedControl : ObjectID::Invalid;
	if (capture.IsValid())
	{
		m_gui.m_mouseCaptureControl = ObjectID::Invalid;
		m_gui.m_mouseCaptureButton  = -1;
	}

	if (hover.IsValid())
	{
		m_gui.m_hoveredControl = ObjectID::Invalid;
	}

	if (clearFocus && focus.IsValid())
	{
		m_gui.m_focusedControl = ObjectID::Invalid;
		++m_gui.m_focusVersion;
	}

	if (Control* target = ObjectDatabase::GetInstance<Control>(capture))
	{
		target->Notification(Control::Notification_GuiInputCancelled);
	}

	Viewport* viewport = ObjectDatabase::GetInstance<Viewport>(viewportID);
	if (viewport == nullptr)
	{
		return;
	}

	if (focus != capture)
	{
		if (Control* target = ObjectDatabase::GetInstance<Control>(focus))
		{
			target->Notification(Control::Notification_GuiInputCancelled);
		}
	}

	viewport = ObjectDatabase::GetInstance<Viewport>(viewportID);
	if (viewport == nullptr)
	{
		return;
	}

	if (clearFocus && viewport->m_gui.m_focusedControl != focus)
	{
		if (Control* target = ObjectDatabase::GetInstance<Control>(focus))
		{
			target->Notification(Control::Notification_FocusExit);
		}
	}

	if (Control* target = ObjectDatabase::GetInstance<Control>(hover))
	{
		target->Notification(Control::Notification_MouseExit);
	}
}

bool Viewport::PropagateGuiInput(ObjectID target, InputEvent const& event)
{
	bool const pointer = event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton
						 || event.m_type == InputEventType::MouseWheel;
	while (Control* control = ResolveGuiControl(target))
	{
		ObjectID parentID = ObjectID::Invalid;
		for (Node* parent = control->GetParent(); parent != nullptr && parent != this; parent = parent->GetParent())
		{
			if (Control* parentControl = dynamic_cast<Control*>(parent))
			{
				parentID = parentControl->GetObjectID();
				break;
			}
		}

		Control::MouseFilter const filter = control->GetMouseFilter();
		InputEvent                 local  = event;
		if (pointer)
		{
			local.m_position = event.m_position - control->GetGlobalPosition();
		}

		if (!pointer || filter != Control::MouseFilter::Ignore)
		{
			if (control->GuiInput(local))
			{
				return true;
			}
		}

		if (!pointer || (filter == Control::MouseFilter::Stop && event.m_type != InputEventType::MouseWheel))
		{
			return pointer;
		}

		target = parentID;
	}

	return false;
}

void Viewport::CollectFocusableControls(Node* node, std::vector<ObjectID>& controls) const
{
	if (node->GetViewport() != this || node->IsQueuedForDeletion() || dynamic_cast<Viewport*>(node) != nullptr)
	{
		return;
	}

	if (CanvasItem* item = dynamic_cast<CanvasItem*>(node); item != nullptr && !item->IsVisibleInHierarchy())
	{
		return;
	}

	if (Control* control = dynamic_cast<Control*>(node);
		control != nullptr && !control->IsDisabled() && control->GetFocusMode() == Control::FocusMode::All)
	{
		controls.push_back(control->GetObjectID());
	}

	for (Node* child : node->GetChildren())
	{
		CollectFocusableControls(child, controls);
	}
}

bool Viewport::ProcessInputEvent(InputEvent const& event)
{
	if (GetSceneTree() == nullptr)
	{
		return false;
	}

	switch (event.m_type)
	{
	case InputEventType::WindowFocus:
	{
		m_gui.m_windowFocused = event.m_focused;
		if (!event.m_focused)
		{
			CancelGuiInteraction(this, false);
		}

		if (Control* focus = ResolveGuiControl(m_gui.m_focusedControl))
		{
			focus->QueueRedraw();
		}

		return false;
	}
	case InputEventType::CursorEnter:
		SetGuiHovered(event.m_entered && m_gui.m_windowFocused ? FindControlAtPosition(event.m_position) : nullptr);
		return false;
	}

	if (!m_gui.m_windowFocused)
	{
		return false;
	}

	// 1) Commit pending layout before converting positional input
	GetSceneTree()->FlushContainerUpdate();

	bool result = false;
	result      = ProcessGuiInputEvent(event);

	return result;
}

bool Viewport::ProcessGuiInputEvent(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::MouseButton:
	case InputEventType::MouseMotion:
	case InputEventType::MouseWheel:
	{
		if (event.m_type == InputEventType::MouseMotion && !m_gui.m_dragging)
		{
			TryStartGuiDrag(event.m_position);
		}

		if (m_gui.m_dragging)
		{
			UpdateGuiDrag(event.m_position);
			if (event.m_type == InputEventType::MouseButton && !event.m_pressed
				&& event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
			{
				ObjectID const target = m_gui.m_dragTarget;
				GuiDragData const data = m_gui.m_dragData;
				CancelGuiDrag();
				if (Control* control = ResolveGuiControl(target);
					control != nullptr && control->CanDropData(event.m_position - control->GetGlobalPosition(), data))
				{
					control = ResolveGuiControl(target);
					if (control != nullptr)
					{
						control->DropData(event.m_position - control->GetGlobalPosition(), data);
					}
				}
			}

			return true;
		}

		Control* targetControl = FindControlAtPosition(event.m_position);
		SetGuiHovered(targetControl);

		if (ResolveGuiControl(m_gui.m_mouseCaptureControl) == nullptr)
		{
			m_gui.m_mouseCaptureControl = ObjectID::Invalid;
			m_gui.m_mouseCaptureButton  = -1;
		}

		ObjectID targetID =
			m_gui.m_mouseCaptureControl.IsValid() ? m_gui.m_mouseCaptureControl : m_gui.m_hoveredControl;
		if (event.m_type == InputEventType::MouseButton)
		{
			// 1) Keep a single-button capture until its matching release
			if (m_gui.m_mouseCaptureControl.IsValid() && event.m_mouseButton != m_gui.m_mouseCaptureButton)
			{
				return true;
			}

			if (event.m_pressed)
			{
				if (!m_gui.m_mouseCaptureControl.IsValid())
				{
					m_gui.m_mouseCaptureControl = targetID;
					m_gui.m_mouseCaptureButton  = targetID.IsValid() ? event.m_mouseButton : -1;
					m_gui.m_dragPressPosition = event.m_position;
					m_gui.m_dragAttempted = false;
				}

				if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
				{
					if (Control* control = ResolveGuiControl(targetID);
						control != nullptr && control->GetFocusMode() != Control::FocusMode::None
						&& !control->IsDisabled())
					{
						SetGuiFocus(control);
					}
				}
			}
			else
			{
				targetID = m_gui.m_mouseCaptureControl;
				if (event.m_mouseButton == m_gui.m_mouseCaptureButton)
				{
					m_gui.m_mouseCaptureControl = ObjectID::Invalid;
					m_gui.m_mouseCaptureButton  = -1;
				}
			}
		}

		bool result = PropagateGuiInput(targetID, event);
		return result;
	}
	case InputEventType::Key:
	case InputEventType::Text:
	{
		if (m_gui.m_dragging && event.m_type == InputEventType::Key && event.m_pressed
			&& event.m_keyCode == GLFW_KEY_ESCAPE)
		{
			CancelGuiDrag();
			return true;
		}

		if (GetGuiPopup() != nullptr)
		{
			return GetGuiPopup()->GuiInput(event);
		}

		if (event.m_type == InputEventType::Key && event.m_keyCode == GLFW_KEY_TAB && event.m_pressed && !event.m_repeat)
		{
			std::vector<ObjectID> controls;
			for (ObjectID id : m_gui.m_rootControls)
			{
				if (Control* root = ResolveGuiControl(id))
				{
					CollectFocusableControls(root, controls);
				}
			}

			if (!controls.empty())
			{
				auto       it        = std::find(controls.begin(), controls.end(), m_gui.m_focusedControl);
				int const  count     = static_cast<int>(controls.size());
				bool const backwards = (event.m_modifiers & GLFW_MOD_SHIFT) != 0;
				int index = it == controls.end() ? (backwards ? 0 : -1) : static_cast<int>(it - controls.begin());
				index     = (index + (backwards ? count - 1 : 1)) % count;
				SetGuiFocus(ResolveGuiControl(controls[index]));
				return true;
			}
		}

		if (Control* focus = ResolveGuiControl(m_gui.m_focusedControl); focus != nullptr && !focus->IsDisabled())
		{
			bool result = PropagateGuiInput(m_gui.m_focusedControl, event);
			return result;
		}

		break;
	}
	}

	return false;
}

void Viewport::SetGuiDragPreview(Control* preview)
{
	if (preview == nullptr || preview->GetParent() != nullptr)
	{
		return;
	}

	if (!m_gui.m_dragStarting && !m_gui.m_dragging)
	{
		preview->QueueFree();
		return;
	}

	if (Control* old = ObjectDatabase::GetInstance<Control>(m_gui.m_dragPreview))
	{
		old->SetVisible(false);
		old->QueueFree();
	}

	std::function<void(Node*)> ignore = [&](Node* node)
	{
		node->SetSerializable(false);
		if (Control* control = dynamic_cast<Control*>(node))
		{
			control->SetMouseFilter(Control::MouseFilter::Ignore);
		}

		for (Node* child : node->GetChildren())
		{
			ignore(child);
		}
	};
	ignore(preview);
	m_gui.m_dragPreviewSize = preview->GetSize();
	m_gui.m_dragPreview = preview->GetObjectID();
	AddNode(preview);
	preview->UpdateSize(m_gui.m_dragPreviewSize);
}

void Viewport::TryStartGuiDrag(Vector2 const& position)
{
	if (m_gui.m_dragAttempted || m_gui.m_mouseCaptureButton != GLFW_MOUSE_BUTTON_LEFT
		|| !m_gui.m_mouseCaptureControl.IsValid() || GetGuiPopup() != nullptr)
	{
		return;
	}

	// Only if the delta distance is greater than 8 pixels
	// we will start the drag operation
	Vector2 const delta = position - m_gui.m_dragPressPosition;
	if (delta.x * delta.x + delta.y * delta.y <= 64.f)
	{
		return;
	}

	m_gui.m_dragAttempted = true;
	ObjectID candidate = m_gui.m_mouseCaptureControl;
	while (Control* control = ResolveGuiControl(candidate))
	{
		ObjectID const parent = dynamic_cast<Control*>(control->GetParent()) != nullptr
			? control->GetParent()->GetObjectID() : ObjectID::Invalid;
		bool const stop = control->GetMouseFilter() == Control::MouseFilter::Stop;
		GuiDragData data;
		m_gui.m_dragStarting = true;
		bool const started = !control->IsDisabled() && control->GetMouseFilter() != Control::MouseFilter::Ignore
			&& control->GetDragData(m_gui.m_dragPressPosition - control->GetGlobalPosition(), data);
		m_gui.m_dragStarting = false;
		if (started && !data.m_type.IsEmpty() && ResolveGuiControl(candidate) != nullptr && GetGuiPopup() == nullptr)
		{
			ObjectID const capture = m_gui.m_mouseCaptureControl;
			m_gui.m_mouseCaptureControl = ObjectID::Invalid;
			m_gui.m_mouseCaptureButton = -1;
			m_gui.m_dragSource = candidate;
			m_gui.m_dragData = data;
			m_gui.m_dragging = true;
			if (Control* pressed = ResolveGuiControl(capture, false))
			{
				pressed->Notification(Control::Notification_GuiInputCancelled);
			}

			if (!m_gui.m_dragPreview.IsValid())
			{
				auto* label = MemNew<Label>();
				label->SetText(data.m_value.IsEmpty() ? data.m_type : data.m_value);
				label->UpdateSize(label->GetCombinedMinimumSize());
				SetGuiDragPreview(label);
			}

			return;
		}

		if (Control* preview = ObjectDatabase::GetInstance<Control>(m_gui.m_dragPreview))
		{
			preview->SetVisible(false);
			preview->QueueFree();
			m_gui.m_dragPreview = ObjectID::Invalid;
		}

		if (stop)
		{
			break;
		}

		candidate = parent;
	}
}

ObjectID Viewport::FindGuiDropTarget(Vector2 const& position) const
{
	Control* hit = FindControlAtPosition(position);
	ObjectID candidate = hit != nullptr ? hit->GetObjectID() : ObjectID::Invalid;
	while (Control* control = ResolveGuiControl(candidate))
	{
		ObjectID const parent = dynamic_cast<Control*>(control->GetParent()) != nullptr
			? control->GetParent()->GetObjectID() : ObjectID::Invalid;
		bool const stop = control->GetMouseFilter() == Control::MouseFilter::Stop;
		if (!control->IsDisabled() && control->GetMouseFilter() != Control::MouseFilter::Ignore
			&& control->CanDropData(position - control->GetGlobalPosition(), m_gui.m_dragData))
		{
			return ResolveGuiControl(candidate) != nullptr ? candidate : ObjectID::Invalid;
		}

		if (stop)
		{
			break;
		}

		candidate = parent;
	}

	return ObjectID::Invalid;
}

void Viewport::SetGuiDragTarget(ObjectID target)
{
	if (target == m_gui.m_dragTarget)
	{
		return;
	}

	ObjectID const previous = m_gui.m_dragTarget;
	m_gui.m_dragTarget = target;
	if (Control* control = ResolveGuiControl(previous, false))
	{
		control->Notification(Control::Notification_DragHoverExit);
	}
}

void Viewport::UpdateGuiDrag(Vector2 const& position)
{
	if (!m_gui.m_dragging)
	{
		return;
	}

	Object* object = ObjectDatabase::GetInstance<Object>(m_gui.m_dragData.m_objectID);
	Node* node = dynamic_cast<Node*>(object);
	if (ResolveGuiControl(m_gui.m_dragSource, false) == nullptr || GetGuiPopup() != nullptr
		|| (m_gui.m_dragData.m_objectID.IsValid() && (object == nullptr || (node != nullptr && node->IsQueuedForDeletion()))))
	{
		CancelGuiDrag();
		return;
	}

	m_gui.m_dragPosition = position;
	Control* hit = FindControlAtPosition(position);
	ObjectID const hover = hit != nullptr ? hit->GetObjectID() : ObjectID::Invalid;
	if (hover != m_gui.m_dragHover)
	{
		ObjectID const previous = m_gui.m_dragHover;
		m_gui.m_dragHover = hover;
		if (Control* control = ResolveGuiControl(previous, false))
		{
			control->Notification(Control::Notification_DragHoverExit);
		}
	}

	SetGuiDragTarget(FindGuiDropTarget(position));
	if (Control* control = ResolveGuiControl(hover))
	{
		control->DragHover(position - control->GetGlobalPosition(), m_gui.m_dragData, hover == m_gui.m_dragTarget);
	}

	if (m_gui.m_dragTarget != hover)
	{
		if (Control* control = ResolveGuiControl(m_gui.m_dragTarget))
		{
			control->DragHover(position - control->GetGlobalPosition(), m_gui.m_dragData, true);
		}
	}

	if (Control* preview = ResolveGuiControl(m_gui.m_dragPreview))
	{
		preview->SetPosition(position + Vector2(14.f, 18.f));
		preview->UpdateSize(m_gui.m_dragPreviewSize);
		if (auto* background = dynamic_cast<ColorRect*>(preview))
		{
			background->SetColor(m_gui.m_dragTarget.IsValid() ? Color(36, 100, 74) : Color(112, 48, 48));
		}
	}
}

void Viewport::CancelGuiDrag()
{
	ObjectID const source = m_gui.m_dragSource;
	ObjectID const target = m_gui.m_dragTarget;
	ObjectID const hover = m_gui.m_dragHover;
	ObjectID const preview = m_gui.m_dragPreview;
	m_gui.m_dragging = false;
	m_gui.m_dragStarting = false;
	m_gui.m_dragSource = ObjectID::Invalid;
	m_gui.m_dragTarget = ObjectID::Invalid;
	m_gui.m_dragHover = ObjectID::Invalid;
	m_gui.m_dragPreview = ObjectID::Invalid;
	m_gui.m_dragData = {};
	m_gui.m_dragAttempted = true;
	if (Control* control = ObjectDatabase::GetInstance<Control>(preview))
	{
		control->SetVisible(false);
		control->QueueFree();
	}

	for (ObjectID id : { source, target, hover })
	{
		if (Control* control = ResolveGuiControl(id, false))
		{
			control->Notification(Control::Notification_DragEnd);
		}
	}
}
