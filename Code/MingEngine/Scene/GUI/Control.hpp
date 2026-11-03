#pragma once

#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Engine/Input/InputEvent.hpp"
#include "MingEngine/Scene/GUI/GUIDefinitions.hpp"

#include "MingEngine/Scene/Resource/Theme.hpp"

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
		Notification_ThemeChanged       = 41,
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
	~Control() override;

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
	virtual bool        GuiInput(InputEvent const& event);
	virtual CursorShape GetCursorShape(Vector2 const& localPosition) const;

	virtual bool GetDragData(Vector2 const& position, GuiDragData& data);
	virtual bool CanDropData(Vector2 const& position, GuiDragData const& data) const;
	virtual void DropData(Vector2 const& position, GuiDragData const& data);
	virtual void DragHover(Vector2 const& position, GuiDragData const& data, bool accepted);

	void SetDragPreview(Control* preview);
	void SetDragCallbacks(
		std::function<bool(Vector2 const&, GuiDragData&)>       source,
		std::function<bool(Vector2 const&, GuiDragData const&)> check,
		std::function<void(Vector2 const&, GuiDragData const&)> drop);
	void SetDragHoverCallback(std::function<void(Vector2 const&, GuiDragData const&, bool)> callback);

	Ref<Theme>         GetTheme() const { return m_theme; }
	std::string const& GetThemeTypeVariation() const { return m_themeTypeVariation; }
	void               SetTheme(Ref<Theme> const& theme);
	void               SetThemeTypeVariation(std::string const& variation);

	// Borrow a detached control's theme ancestry without inheriting its overrides or variation.
	// e.g. A viewport popup uses its triggering OptionButton as its theme source.
	Control* GetThemeSource() const;
	bool     SetThemeSource(Control* source);

	// Refresh parsed values, not the source Theme; setters and subscriptions call this automatically.
	// e.g. Editing a shared ThemeStyleBox updates cache, minimum size and drawing on its subscribed controls.
	void RefreshTheme();
	void AddThemeChangedCallback(std::function<void()> callback);

	Color GetThemeColor(std::string const& name, std::string const& type = {}) const;
	void  AddThemeColorOverride(std::string const& name, Color const& value);
	void  RemoveThemeColorOverride(std::string const& name);
	bool  HasThemeColorOverride(std::string const& name) const;

	int  GetThemeConstant(std::string const& name, std::string const& type = {}) const;
	void AddThemeConstantOverride(std::string const& name, int value);
	void RemoveThemeConstantOverride(std::string const& name);
	bool HasThemeConstantOverride(std::string const& name) const;

	Ref<FontResource> GetThemeFont(std::string const& name, std::string const& type = {}) const;
	void              AddThemeFontOverride(std::string const& name, Ref<FontResource> const& value);
	void              RemoveThemeFontOverride(std::string const& name);
	bool              HasThemeFontOverride(std::string const& name) const;

	int  GetThemeFontSize(std::string const& name, std::string const& type = {}) const;
	void AddThemeFontSizeOverride(std::string const& name, int value);
	void RemoveThemeFontSizeOverride(std::string const& name);
	bool HasThemeFontSizeOverride(std::string const& name) const;

	Ref<TextureResource> GetThemeIcon(std::string const& name, std::string const& type = {}) const;
	void                 AddThemeIconOverride(std::string const& name, Ref<TextureResource> const& value);
	void                 RemoveThemeIconOverride(std::string const& name);
	bool                 HasThemeIconOverride(std::string const& name) const;

	Ref<ThemeStyleBox> GetThemeStyleBox(std::string const& name, std::string const& type = {}) const;
	void               AddThemeStyleBoxOverride(std::string const& name, Ref<ThemeStyleBox> const& value);
	void               RemoveThemeStyleBoxOverride(std::string const& name);
	bool               HasThemeStyleBoxOverride(std::string const& name) const;

	template <Theme::DataType Type>
	auto GetThemeItem(std::string const& name) const
	{
		if constexpr (Type == Theme::DataType::Color)
		{
			return GetThemeColor(name);
		}
		else if constexpr (Type == Theme::DataType::Constant)
		{
			return GetThemeConstant(name);
		}
		else if constexpr (Type == Theme::DataType::Font)
		{
			return GetThemeFont(name);
		}
		else if constexpr (Type == Theme::DataType::FontSize)
		{
			return GetThemeFontSize(name);
		}
		else if constexpr (Type == Theme::DataType::Icon)
		{
			return GetThemeIcon(name);
		}
		else if constexpr (Type == Theme::DataType::StyleBox)
		{
			return GetThemeStyleBox(name);
		}
	}

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
	virtual void OnThemeChanged() {}
	virtual void OnThemeSourceLost() {}
	void         DrawThemeStyleBox(Ref<ThemeStyleBox> const& box, AABB2 const& rect);

	void OnNotification(int notification);

private:
	void                     RefreshThemeBranch();
	void                     ReleaseThemeDependents();
	Control const*           GetThemeParent() const;
	void                     ReleaseThemeSubscriptions();
	void                     RebuildThemeSubscriptions();
	std::vector<std::string> GetThemeTypes(std::string const& type) const;
	void                     SetSize(Vector2 const& size);

private:
	ObjectID                           m_themeSource = ObjectID::Invalid;
	std::vector<std::function<void()>> m_themeChangedCallbacks;
	std::set<ObjectID>                 m_themeDependents;
	Ref<Theme>                         m_theme;
	std::string                        m_themeTypeVariation;
	std::vector<Ref<Theme>>            m_themeSubscriptions;
	bool                               m_themeInitialized    = false;
	bool                               m_refreshingTheme     = false;
	bool                               m_pendingThemeRefresh = false;

	std::map<std::string, Color>                m_colorOverrides;
	std::map<std::string, int>                  m_constantOverrides;
	std::map<std::string, Ref<FontResource>>    m_fontOverrides;
	std::map<std::string, int>                  m_fontSizeOverrides;
	std::map<std::string, Ref<TextureResource>> m_iconOverrides;
	std::map<std::string, Ref<ThemeStyleBox>>   m_styleBoxOverrides;

	Vector2 m_position          = Vector2::Zero;
	Vector2 m_size              = Vector2::Zero;
	Vector2 m_customMinimumSize = Vector2::Zero;

	int         m_horizontalSizeFlags = Size_Fill;
	int         m_verticalSizeFlags   = Size_Fill;
	MouseFilter m_mouseFilter         = MouseFilter::Stop;
	FocusMode   m_focusMode           = FocusMode::None;
	bool        m_clipContents        = false;

	std::function<bool(Vector2 const&, GuiDragData&)>             m_dragSource;
	std::function<bool(Vector2 const&, GuiDragData const&)>       m_dropCheck;
	std::function<void(Vector2 const&, GuiDragData const&)>       m_drop;
	std::function<void(Vector2 const&, GuiDragData const&, bool)> m_dragHover;
};
