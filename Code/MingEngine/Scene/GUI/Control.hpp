#pragma once

#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Engine/Input/InputEvent.hpp"
#include "MingEngine/Scene/GUI/GUIDefinitions.hpp"

#include <functional>

// Control inherits from CanvasItem, which means Control can call all these render functions
// but Control has its own unique features, like size, position, minimum size, etc.
// this is because Control is a UI element, which means it will take the whole screen as its target
// which is very different from node 2d
class Control : public CanvasItem
{
	MCLASS(Control, CanvasItem);

public:
	enum
	{
		Notification_Resized            = 31,
		Notification_MinimumSizeChanged = 32,
		Notification_MouseEnter         = 35,
		Notification_MouseExit          = 36,
		Notification_FocusEnter         = 37,
		Notification_FocusExit          = 38,
		Notification_GuiInputCancelled  = 39,
		Notification_DragHoverExit      = 70,
		Notification_DragEnd            = 71
	};

	enum class MouseFilter
	{
		Stop,
		Pass,
		Ignore
	};
	enum class FocusMode
	{
		None,
		Click,
		All
	};

	// Without Fill, the control uses its minimum size and defaults to begin alignment.
	// e.g. Size_ExpandFill fills its allocation, while Size_Expand alone keeps the minimum size at the beginning.
	enum SizeFlags
	{
		Size_ShrinkBegin  = 0,
		Size_Fill         = 1 << 0,
		Size_Expand       = 1 << 1,
		Size_ShrinkCenter = 1 << 2,
		Size_ShrinkEnd    = 1 << 3,

		Size_ExpandFill = Size_Expand | Size_Fill
	};

public:
	static void BindMethods() {}

	// We use Update Size instead of SetSize here, because we have a custom minimum size
	// which is user defined size data
	// and update size is used by container to update the size
	// in this case, we do not want to change the custom minimum size
	// and actually, we event want the minimum size to limit the size
	// so here has some judge logic instead of just set the size directly
	void         UpdateSize(Vector2 const& newSize);
	virtual bool IsPointInside(Vector2 const& localPosition) const;
	// If GuiInput returns true
	// it means the event is consumed and should not propagate to other controls.
	virtual bool GuiInput(InputEvent const& event);

	virtual bool GetDragData(Vector2 const& position, GuiDragData& data);
	virtual bool CanDropData(Vector2 const& position, GuiDragData const& data) const;
	virtual void DropData(Vector2 const& position, GuiDragData const& data);
	virtual void DragHover(Vector2 const& position, GuiDragData const& data, bool accepted);

	void SetDragPreview(Control* preview);
	void SetDragCallbacks(
		std::function<bool(Vector2 const&, GuiDragData&)> source,
		std::function<bool(Vector2 const&, GuiDragData const&)> check,
		std::function<void(Vector2 const&, GuiDragData const&)> drop);
	void SetDragHoverCallback(std::function<void(Vector2 const&, GuiDragData const&, bool)> callback);

	virtual bool IsDisabled() const { return false; }
	MouseFilter  GetMouseFilter() const { return m_mouseFilter; }
	FocusMode    GetFocusMode() const { return m_focusMode; }

	void SetMouseFilter(MouseFilter filter);
	void SetFocusMode(FocusMode mode);
	void GrabFocus();
	void ReleaseFocus();
	bool HasFocus() const;
	void SetClipContents(bool enabled);
	bool IsClippingContents() const { return m_clipContents; }

	Vector2 GetSize() const { return m_size; }
	AABB2   GetLocalRect() const { return AABB2(Vector2::Zero, m_size); }
	Vector2 GetLocalPosition() const override { return m_position; }

	virtual Vector2 GetInternalMinimumSize() const;

	Vector2 GetCustomMinimumSize() const { return m_customMinimumSize; }

	Vector2 GetCombinedMinimumSize() const;

	int GetHorizontalSizeFlags() const { return m_horizontalSizeFlags; }
	int GetVerticalSizeFlags() const { return m_verticalSizeFlags; }

	void SetPosition(Vector2 const& position);
	void SetCustomMinimumSize(Vector2 const& size);

	void SetHorizontalSizeFlags(int flags);
	void SetVerticalSizeFlags(int flags);

	// Propagate is spread the message along tree structure
	void PropagateMinimumSizeChanged();

protected:
	void OnNotification(int notification);

private:
	void SetSize(Vector2 const& size);

private:
	Vector2 m_position          = Vector2::Zero;
	Vector2 m_size              = Vector2::Zero;
	Vector2 m_customMinimumSize = Vector2::Zero;

	int         m_horizontalSizeFlags = Size_Fill;
	int         m_verticalSizeFlags   = Size_Fill;
	MouseFilter m_mouseFilter         = MouseFilter::Stop;
	FocusMode   m_focusMode           = FocusMode::None;
	bool        m_clipContents        = false;
	std::function<bool(Vector2 const&, GuiDragData&)> m_dragSource;
	std::function<bool(Vector2 const&, GuiDragData const&)> m_dropCheck;
	std::function<void(Vector2 const&, GuiDragData const&)> m_drop;
	std::function<void(Vector2 const&, GuiDragData const&, bool)> m_dragHover;
};
