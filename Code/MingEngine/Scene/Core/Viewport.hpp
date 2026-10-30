#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Engine/Input/InputEvent.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/GUI/GUIDefinitions.hpp"

#include <set>

class Camera3D;
class Control;

// Form render side
// Viewport is basically a canvas, a paper, where the camera and its child node can render to
// It is a node with a renderable texture
class Viewport : public Node
{
	MCLASS(Viewport, Node);

protected:
	struct GuiContext
	{
		std::vector<ObjectID> m_rootControls;

		ObjectID m_hoveredControl = ObjectID::Invalid;

		// We will have a mouseCaptureControl
		// which means the mouse is captured by this control
		// all the mouse events will be sent to this control until the mouse is released
		ObjectID m_mouseCaptureControl = ObjectID::Invalid;
		int      m_mouseCaptureButton  = -1;

		ObjectID m_focusedControl = ObjectID::Invalid;
		uint64_t m_focusVersion   = 0;
		bool     m_windowFocused  = true;

		ObjectID m_popupControl = ObjectID::Invalid;

		Vector2 m_dragPressPosition = Vector2::Zero;
		bool    m_dragAttempted     = false;

		Vector2     m_dragPosition = Vector2::Zero;
		ObjectID    m_dragSource   = ObjectID::Invalid;
		ObjectID    m_dragTarget   = ObjectID::Invalid;
		ObjectID    m_dragHover    = ObjectID::Invalid;
		GuiDragData m_dragData;
		bool        m_dragStarting = false;
		bool        m_dragging     = false;

		Vector2  m_dragPreviewSize = Vector2::Zero;
		ObjectID m_dragPreview     = ObjectID::Invalid;
	};


public:
	Viewport();
	~Viewport() override;

	IntVec2 GetOutputResolution() const;
	void    UpdateResolution(IntVec2 dimensions);

	void      AddCamera(Camera3D* camera);
	void      RemoveCamera(Camera3D* camera);
	void      SetCurrentCamera(Camera3D* camera);
	Camera3D* GetCurrentCamera() const;
	void      ChangeToNextCamera();

	// All Viewport render state lives on the RenderServer and is addressed by this RID.
	RID GetViewportRID() const { return m_viewportRID; }
	RID GetCanvasLayerRID() const { return m_canvasLayerRID; }

	// Scenario that owns this Viewport's instances on the RenderServer.
	RID GetScenarioRID() const { return m_scenarioRID; }

	// When Window was triggered, it will call ProcessInputEvent to pass this event to Viewport
	// ProcessInputEvent will make sure the big level things, such as the windows focus and hover
	// ProcessGuiInputEvent will make sure the middle level things, such as the gui focus
	// And finally, the ProcessGuiInputEvent will make sure the small level things
	// and in here, the event finally pass to the specific Control, and will be handled along the tree structure
	bool ProcessInputEvent(InputEvent const& event);
	bool ProcessGuiInputEvent(InputEvent const& event);

	void AddRootControl(Control* control);
	void RemoveRootControl(Control* control);

	bool HasGuiFocus(Control const* control) const;
	bool IsWindowFocused() const { return m_gui.m_windowFocused; }

	void SetGuiHovered(Control* control);
	void SetGuiFocus(Control* control);
	Control* GetGuiFocus() const { return ResolveGuiControl(m_gui.m_focusedControl); }
	Control* GetGuiPopup() const { return ResolveGuiControl(m_gui.m_popupControl); }
	void SetGuiPopup(Control* control);
	bool HasGuiMouseCapture() const { return ResolveGuiControl(m_gui.m_mouseCaptureControl) != nullptr; }

	bool IsGuiDragging() const { return m_gui.m_dragging; }
	GuiDragData const& GetGuiDragData() const { return m_gui.m_dragData; }
	Vector2 GetGuiDragPosition() const { return m_gui.m_dragPosition; }
	Control* GetGuiDragTarget() const { return ResolveGuiControl(m_gui.m_dragTarget); }
	void SetGuiDragPreview(Control* preview);
	void UpdateGuiDrag(Vector2 const& position);
	void CancelGuiDrag();

	void CancelGuiInteraction(Node* subtree, bool clearFocus = true, bool cancelDrag = true);

protected:
	void OnNotification(int notification);

	void SetResolution(IntVec2 dimensions);

	Control* ResolveGuiControl(ObjectID id, bool requireVisible = true) const;
	Control* FindControlAtPosition(Vector2 const& position) const;
	Control* FindControlInSubtree(Node* node, Vector2 const& position) const;

	void CollectFocusableControls(Node* node, std::vector<ObjectID>& controls) const;

	bool PropagateGuiInput(ObjectID target, InputEvent const& event);
	void TryStartGuiDrag(Vector2 const& position);
	ObjectID FindGuiDropTarget(Vector2 const& position) const;
	void SetGuiDragTarget(ObjectID target);

protected:
	Camera3D*           m_currentCamera = nullptr;
	std::set<Camera3D*> m_cameras;

	RID     m_viewportRID      = RID::Invalid;
	RID     m_canvasLayerRID   = RID::Invalid;
	RID     m_scenarioRID      = RID::Invalid;
	IntVec2 m_outputResolution = IntVec2::Zero;


	GuiContext m_gui;
};

// SubViewport is basically viewport and mainly used for off screen rendering
class SubViewport : public Viewport
{
	MCLASS(SubViewport, Viewport);

protected:
	static void BindMethods() {}
};
