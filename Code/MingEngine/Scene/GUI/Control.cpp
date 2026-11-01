#include "MingEngine/Scene/GUI/Control.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include <algorithm>

bool Control::IsPointInside(Vector2 const& localPosition) const
{
	return m_size.x > 0.f && m_size.y > 0.f && localPosition.x >= 0.f && localPosition.y >= 0.f
		   && localPosition.x < m_size.x && localPosition.y < m_size.y;
}

bool Control::GuiInput([[maybe_unused]] InputEvent const& event)
{
	return false;
}

CursorShape Control::GetCursorShape([[maybe_unused]] Vector2 const& localPosition) const
{
	return CursorShape::Arrow;
}

bool Control::GetDragData(Vector2 const& position, GuiDragData& data)
{
	auto callback = m_dragSource;
	return callback && callback(position, data);
}

bool Control::CanDropData(Vector2 const& position, GuiDragData const& data) const
{
	auto callback = m_dropCheck;
	return callback && callback(position, data);
}

void Control::DropData(Vector2 const& position, GuiDragData const& data)
{
	auto callback = m_drop;
	if (callback)
	{
		callback(position, data);
	}
}

void Control::DragHover(Vector2 const& position, GuiDragData const& data, bool accepted)
{
	auto callback = m_dragHover;
	if (callback)
	{
		callback(position, data, accepted);
	}
}

void Control::SetDragPreview(Control* preview)
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->SetGuiDragPreview(preview);
	}
}

void Control::SetDragCallbacks(
	std::function<bool(Vector2 const&, GuiDragData&)> source,
	std::function<bool(Vector2 const&, GuiDragData const&)> check,
	std::function<void(Vector2 const&, GuiDragData const&)> drop)
{
	m_dragSource = std::move(source);
	m_dropCheck = std::move(check);
	m_drop = std::move(drop);
}

void Control::SetDragHoverCallback(std::function<void(Vector2 const&, GuiDragData const&, bool)> callback)
{
	m_dragHover = std::move(callback);
}

void Control::SetMouseFilter(MouseFilter filter)
{
	if (m_mouseFilter == filter)
	{
		return;
	}

	m_mouseFilter = filter;
	if (filter == MouseFilter::Ignore && GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this, false);
	}
}

void Control::SetFocusMode(FocusMode mode)
{
	m_focusMode = mode;
	if (mode == FocusMode::None)
	{
		ReleaseFocus();
	}
}

void Control::GrabFocus()
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->SetGuiFocus(this);
	}
}

void Control::ReleaseFocus()
{
	if (HasFocus())
	{
		GetViewport()->SetGuiFocus(nullptr);
	}
}

bool Control::HasFocus() const
{
	return GetViewport() != nullptr && GetViewport()->HasGuiFocus(this);
}

void Control::SetClipContents(bool enabled)
{
	m_clipContents = enabled;
	SetCanvasClip(enabled, m_size);
}

void Control::UpdateSize(Vector2 const& newSize)
{
	Vector2 const minimumSize = GetCombinedMinimumSize();

	Vector2 const updatedSize(
		newSize.x < minimumSize.x ? minimumSize.x : newSize.x,
		newSize.y < minimumSize.y ? minimumSize.y : newSize.y);

	if (m_size == updatedSize)
	{
		return;
	}

	SetSize(updatedSize);

	QueueRedraw();
}

Vector2 Control::GetInternalMinimumSize() const
{
	return Vector2::Zero;
}

Vector2 Control::GetCombinedMinimumSize() const
{
	Vector2 const minimumSize = GetInternalMinimumSize();

	return Vector2(
		minimumSize.x < m_customMinimumSize.x ? m_customMinimumSize.x : minimumSize.x,
		minimumSize.y < m_customMinimumSize.y ? m_customMinimumSize.y : minimumSize.y);
}

void Control::SetPosition(Vector2 const& position)
{
	m_position = position;
	SyncPosition();
}

void Control::SetSize(Vector2 const& size)
{
	m_size = size;
	SetCanvasClip(m_clipContents, m_size);
	Notification(Notification_Resized);
}

void Control::SetCustomMinimumSize(Vector2 const& size)
{
	Vector2 const newMinimumSize(size.x < 0.0f ? 0.0f : size.x, size.y < 0.0f ? 0.0f : size.y);

	if (m_customMinimumSize == newMinimumSize)
	{
		return;
	}

	m_customMinimumSize = newMinimumSize;
	PropagateMinimumSizeChanged();
}

void Control::SetHorizontalSizeFlags(int flags)
{
	if (m_horizontalSizeFlags == flags)
	{
		return;
	}

	m_horizontalSizeFlags = flags;

	if (Container* parent = dynamic_cast<Container*>(GetParent()))
	{
		parent->MarkDirty();
	}
}

void Control::SetVerticalSizeFlags(int flags)
{
	if (m_verticalSizeFlags == flags)
	{
		return;
	}

	m_verticalSizeFlags = flags;

	if (Container* parent = dynamic_cast<Container*>(GetParent()))
	{
		parent->MarkDirty();
	}
}

void Control::PropagateMinimumSizeChanged()
{
	UpdateSize(m_size);

	// Notification spread the message along the inherit chain, from child to parent
	Notification(Notification_MinimumSizeChanged);

	if (Control* parent = dynamic_cast<Control*>(GetParent()))
	{
		parent->PropagateMinimumSizeChanged();
	}
}

void Control::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_PostInitialize:
		// Refresh only after the most-derived object and its private ThemeCache are constructed.
		// e.g. MemNew<Button>() reaches this after Button has constructed its own ThemeCache.
		m_themeInitialized = true;
		ThemeDatabase::RegisterControl(this);
		RefreshTheme();
		break;
	case Notification_Parented:
	case Notification_Unparented:
		RefreshThemeBranch();
		break;
	case Notification_ThemeChanged:
		RefreshTheme();
		break;
	case Notification_PreDelete:
		ReleaseThemeDependents();
		SetThemeSource(nullptr);
		m_themeInitialized = false;
		ReleaseThemeSubscriptions();
		ThemeDatabase::UnregisterControl(this);
		break;
	case Notification_DragHoverExit:
	case Notification_DragEnd:
		DragHover(Vector2(-1.f, -1.f), {}, false);
		break;
	case Notification_EnterTree:
	{
		if (m_data.m_viewport != nullptr && GetParentItem() == nullptr)
		{
			m_data.m_viewport->AddRootControl(this);
		}

		break;
	}

	case Notification_ExitTree:
	{
		ReleaseThemeDependents();
		if (m_data.m_viewport != nullptr)
		{
			m_data.m_viewport->RemoveRootControl(this);
			m_data.m_viewport->CancelGuiInteraction(this);
		}

		break;
	}

	case Notification_VisibilityChanged:
	{
		if (Control* parent = dynamic_cast<Control*>(GetParent()))
		{
			parent->PropagateMinimumSizeChanged();
		}

		break;
	}
	}
}

Control::~Control()
{
	ReleaseThemeDependents();
	SetThemeSource(nullptr);
	m_themeInitialized = false;
	ReleaseThemeSubscriptions();
	ThemeDatabase::UnregisterControl(this);
	for (auto const& item : m_styleBoxOverrides)
	{
		item.second->RemoveUser(GetObjectID());
	}
}

void Control::SetTheme(Ref<Theme> const& theme)
{
	if (m_theme != theme)
	{
		m_theme = theme;
		RefreshThemeBranch();
	}
}

void Control::SetThemeTypeVariation(std::string const& variation)
{
	if (m_themeTypeVariation != variation)
	{
		m_themeTypeVariation = variation;
		RefreshTheme();
	}
}

void Control::ReleaseThemeSubscriptions()
{
	for (auto const& theme : m_themeSubscriptions)
	{
		theme->RemoveUser(GetObjectID());
	}

	m_themeSubscriptions.clear();
}

void Control::RebuildThemeSubscriptions()
{
	ReleaseThemeSubscriptions();
	auto add = [this](Ref<Theme> const& theme)
	{
		if (theme.IsValid()
			&& std::find(m_themeSubscriptions.begin(), m_themeSubscriptions.end(), theme) == m_themeSubscriptions.end())
		{
			m_themeSubscriptions.push_back(theme);
			theme->AddUser(GetObjectID());
		}
	};

	Node const* contextNode = this;
	std::set<ObjectID> visited;
	for (Control const* current = this; current != nullptr && visited.insert(current->GetObjectID()).second;
		 current = current->GetThemeParent())
	{
		add(current->m_theme);
		if (Control* source = current->GetThemeSource())
		{
			contextNode = source;
		}
	}

	for (auto const& theme : ThemeDatabase::GetThemeContext(contextNode))
	{
		add(theme);
	}

	// Subscribe to missing-item sources too, so adding an item later invalidates this cache.
	// e.g. A parent Theme without font_color must still notify a child when font_color is added.
	add(ThemeDatabase::GetDefaultTheme());
}

std::vector<std::string> Control::GetThemeTypes(std::string const& type) const
{
	std::vector<std::string> result;
	auto                     add = [&result](std::string const& name)
	{
		if (!name.empty() && std::find(result.begin(), result.end(), name) == result.end())
		{
			result.push_back(name);
		}
	};

	bool const  ownType = type.empty() || type == GetClassName() || type == m_themeTypeVariation;
	std::string current = ownType ? m_themeTypeVariation : type;
	if (!current.empty())
	{
		for (auto const& theme : m_themeSubscriptions)
		{
			if (!theme->GetTypeVariationBase(current).empty())
			{
				std::set<std::string> visited;
				while (!current.empty() && visited.insert(current).second)
				{
					add(current);
					std::string next = theme->GetTypeVariationBase(current);
					if (next.empty())
					{
						break;
					}

					current = next;
				}

				break;
			}
		}

		add(current);
		while (auto* info = ClassDatabase::GetClassInfo(current))
		{
			current = info->m_parentClassName;
			add(current);
		}
	}

	if (ownType)
	{
		current = GetClassName();
		while (!current.empty())
		{
			add(current);
			auto* info = ClassDatabase::GetClassInfo(current);
			current    = info != nullptr ? info->m_parentClassName : std::string();
		}
	}

	return result;
}

void Control::RefreshTheme()
{
	if (!m_themeInitialized || IsPreDeleting() || IsQueuedForDeletion())
	{
		return;
	}

	m_pendingThemeRefresh = true;
	if (m_refreshingTheme)
	{
		return;
	}

	m_refreshingTheme = true;
	while (m_pendingThemeRefresh)
	{
		m_pendingThemeRefresh = false;
		// 1) Rebuild dependencies before resolving registered cache fields.
		RebuildThemeSubscriptions();
		ThemeDatabase::UpdateClassInstanceItems(this);
		// 2) Apply derived state only after every inherited cache has been populated.
		OnThemeChanged();
		PropagateMinimumSizeChanged();
		QueueRedraw();
	}

	m_refreshingTheme = false;
	auto const dependents = m_themeDependents;
	for (ObjectID id : dependents)
	{
		if (Control* dependent = ObjectDatabase::GetInstance<Control>(id))
		{
			dependent->RefreshThemeBranch();
		}
	}
}

void Control::RefreshThemeBranch()
{
	ThemeDatabase::RefreshThemeContexts(this);
}

void Control::DrawThemeStyleBox(Ref<ThemeStyleBox> const& box, AABB2 const& rect)
{
	if (!box.IsValid() || rect.GetWidth() <= 0.f || rect.GetHeight() <= 0.f)
	{
		return;
	}

	DrawRect(rect, box->GetBackgroundColor());
	using Side         = ThemeStyleBox::Side;
	float const left   = std::min(rect.GetWidth(), box->GetBorderWidth(Side::Left));
	float const right  = std::min(rect.GetWidth(), box->GetBorderWidth(Side::Right));
	float const top    = std::min(rect.GetHeight(), box->GetBorderWidth(Side::Top));
	float const bottom = std::min(rect.GetHeight(), box->GetBorderWidth(Side::Bottom));
	DrawRect(AABB2(rect.m_mins, Vector2(rect.m_mins.x + left, rect.m_maxs.y)), box->GetBorderColor());
	DrawRect(AABB2(Vector2(rect.m_maxs.x - right, rect.m_mins.y), rect.m_maxs), box->GetBorderColor());
	DrawRect(AABB2(rect.m_mins, Vector2(rect.m_maxs.x, rect.m_mins.y + top)), box->GetBorderColor());
	DrawRect(AABB2(Vector2(rect.m_mins.x, rect.m_maxs.y - bottom), rect.m_maxs), box->GetBorderColor());
}

Color Control::GetThemeColor(std::string const& name, std::string const& type) const
{
	if (type.empty() || type == GetClassName() || type == m_themeTypeVariation)
	{
		auto found = m_colorOverrides.find(name);
		if (found != m_colorOverrides.end())
		{
			return found->second;
		}
	}

	auto const types = GetThemeTypes(type);
	for (auto const& theme : m_themeSubscriptions)
	{
		for (auto const& themeType : types)
		{
			if (theme->HasColor(name, themeType))
			{
				return theme->GetColor(name, themeType);
			}
		}
	}

	return Color::White;
}

void Control::AddThemeColorOverride(std::string const& name, Color const& value)
{
	auto found = m_colorOverrides.find(name);
	if (found != m_colorOverrides.end() && found->second == value)
	{
		return;
	}

	m_colorOverrides[name] = value;
	RefreshTheme();
}

void Control::RemoveThemeColorOverride(std::string const& name)
{
	if (m_colorOverrides.erase(name) != 0)
	{
		RefreshTheme();
	}
}

bool Control::HasThemeColorOverride(std::string const& name) const
{
	return m_colorOverrides.find(name) != m_colorOverrides.end();
}

int Control::GetThemeConstant(std::string const& name, std::string const& type) const
{
	if (type.empty() || type == GetClassName() || type == m_themeTypeVariation)
	{
		auto found = m_constantOverrides.find(name);
		if (found != m_constantOverrides.end())
		{
			return found->second;
		}
	}

	auto const types = GetThemeTypes(type);
	for (auto const& theme : m_themeSubscriptions)
	{
		for (auto const& themeType : types)
		{
			if (theme->HasConstant(name, themeType))
			{
				return theme->GetConstant(name, themeType);
			}
		}
	}

	return 0;
}

void Control::AddThemeConstantOverride(std::string const& name, int value)
{
	auto found = m_constantOverrides.find(name);
	if (found != m_constantOverrides.end() && found->second == value)
	{
		return;
	}

	m_constantOverrides[name] = value;
	RefreshTheme();
}

void Control::RemoveThemeConstantOverride(std::string const& name)
{
	if (m_constantOverrides.erase(name) != 0)
	{
		RefreshTheme();
	}
}

bool Control::HasThemeConstantOverride(std::string const& name) const
{
	return m_constantOverrides.find(name) != m_constantOverrides.end();
}

Ref<FontResource> Control::GetThemeFont(std::string const& name, std::string const& type) const
{
	if (type.empty() || type == GetClassName() || type == m_themeTypeVariation)
	{
		auto found = m_fontOverrides.find(name);
		if (found != m_fontOverrides.end())
		{
			return found->second;
		}
	}

	auto const types = GetThemeTypes(type);
	for (auto const& theme : m_themeSubscriptions)
	{
		for (auto const& themeType : types)
		{
			if (theme->HasFont(name, themeType))
			{
				return theme->GetFont(name, themeType);
			}
		}

		if (theme->HasDefaultFont())
		{
			return theme->GetDefaultFont();
		}
	}

	return {};
}

void Control::AddThemeFontOverride(std::string const& name, Ref<FontResource> const& value)
{
	// An explicit empty font suppresses inheritance; remove the override to inherit again.
	// e.g. AddThemeFontOverride("font", {}) intentionally suppresses text until the override is removed.
	auto found = m_fontOverrides.find(name);
	if (found != m_fontOverrides.end() && found->second == value)
	{
		return;
	}

	m_fontOverrides[name] = value;
	RefreshTheme();
}

void Control::RemoveThemeFontOverride(std::string const& name)
{
	if (m_fontOverrides.erase(name) != 0)
	{
		RefreshTheme();
	}
}

bool Control::HasThemeFontOverride(std::string const& name) const
{
	return m_fontOverrides.find(name) != m_fontOverrides.end();
}

int Control::GetThemeFontSize(std::string const& name, std::string const& type) const
{
	if (type.empty() || type == GetClassName() || type == m_themeTypeVariation)
	{
		auto found = m_fontSizeOverrides.find(name);
		if (found != m_fontSizeOverrides.end())
		{
			return found->second;
		}
	}

	auto const types = GetThemeTypes(type);
	for (auto const& theme : m_themeSubscriptions)
	{
		for (auto const& themeType : types)
		{
			if (theme->HasFontSize(name, themeType))
			{
				return theme->GetFontSize(name, themeType);
			}
		}

		if (theme->HasDefaultFontSize())
		{
			return theme->GetDefaultFontSize();
		}
	}

	return 16;
}

void Control::AddThemeFontSizeOverride(std::string const& name, int value)
{
	if (value <= 0)
	{
		return;
	}

	auto found = m_fontSizeOverrides.find(name);
	if (found != m_fontSizeOverrides.end() && found->second == value)
	{
		return;
	}

	m_fontSizeOverrides[name] = value;
	RefreshTheme();
}

void Control::RemoveThemeFontSizeOverride(std::string const& name)
{
	if (m_fontSizeOverrides.erase(name) != 0)
	{
		RefreshTheme();
	}
}

bool Control::HasThemeFontSizeOverride(std::string const& name) const
{
	return m_fontSizeOverrides.find(name) != m_fontSizeOverrides.end();
}

Ref<TextureResource> Control::GetThemeIcon(std::string const& name, std::string const& type) const
{
	if (type.empty() || type == GetClassName() || type == m_themeTypeVariation)
	{
		auto found = m_iconOverrides.find(name);
		if (found != m_iconOverrides.end())
		{
			return found->second;
		}
	}

	auto const types = GetThemeTypes(type);
	for (auto const& theme : m_themeSubscriptions)
	{
		for (auto const& themeType : types)
		{
			if (theme->HasIcon(name, themeType))
			{
				return theme->GetIcon(name, themeType);
			}
		}
	}

	return {};
}

void Control::AddThemeIconOverride(std::string const& name, Ref<TextureResource> const& value)
{
	if (!value.IsValid())
	{
		RemoveThemeIconOverride(name);
		return;
	}

	auto found = m_iconOverrides.find(name);
	if (found != m_iconOverrides.end() && found->second == value)
	{
		return;
	}

	m_iconOverrides[name] = value;
	RefreshTheme();
}

void Control::RemoveThemeIconOverride(std::string const& name)
{
	if (m_iconOverrides.erase(name) != 0)
	{
		RefreshTheme();
	}
}

bool Control::HasThemeIconOverride(std::string const& name) const
{
	return m_iconOverrides.find(name) != m_iconOverrides.end();
}

Ref<ThemeStyleBox> Control::GetThemeStyleBox(std::string const& name, std::string const& type) const
{
	if (type.empty() || type == GetClassName() || type == m_themeTypeVariation)
	{
		auto found = m_styleBoxOverrides.find(name);
		if (found != m_styleBoxOverrides.end())
		{
			return found->second;
		}
	}

	auto const types = GetThemeTypes(type);
	for (auto const& theme : m_themeSubscriptions)
	{
		for (auto const& themeType : types)
		{
			if (theme->HasStyleBox(name, themeType))
			{
				return theme->GetStyleBox(name, themeType);
			}
		}
	}

	return {};
}

void Control::AddThemeStyleBoxOverride(std::string const& name, Ref<ThemeStyleBox> const& value)
{
	if (!value.IsValid())
	{
		RemoveThemeStyleBoxOverride(name);
		return;
	}

	auto found = m_styleBoxOverrides.find(name);
	if (found != m_styleBoxOverrides.end() && found->second == value)
	{
		return;
	}

	if (found != m_styleBoxOverrides.end())
	{
		found->second->RemoveUser(GetObjectID());
	}

	value->AddUser(GetObjectID());
	m_styleBoxOverrides[name] = value;
	RefreshTheme();
}

void Control::RemoveThemeStyleBoxOverride(std::string const& name)
{
	auto found = m_styleBoxOverrides.find(name);
	if (found != m_styleBoxOverrides.end())
	{
		found->second->RemoveUser(GetObjectID());
	}

	if (m_styleBoxOverrides.erase(name) != 0)
	{
		RefreshTheme();
	}
}

bool Control::HasThemeStyleBoxOverride(std::string const& name) const
{
	return m_styleBoxOverrides.find(name) != m_styleBoxOverrides.end();
}

Control* Control::GetThemeSource() const
{
	return ObjectDatabase::GetInstance<Control>(m_themeSource);
}

Control const* Control::GetThemeParent() const
{
	if (Control* source = GetThemeSource())
	{
		return source;
	}

	return dynamic_cast<Control*>(GetParent());
}

bool Control::SetThemeSource(Control* source)
{
	std::set<ObjectID> visited;
	for (Control const* current = source; current != nullptr; current = current->GetThemeParent())
	{
		if (current == this || !visited.insert(current->GetObjectID()).second)
		{
			return false;
		}
	}

	ObjectID const sourceID = source != nullptr ? source->GetObjectID() : ObjectID::Invalid;
	if (m_themeSource == sourceID)
	{
		return true;
	}

	if (Control* previous = GetThemeSource())
	{
		previous->m_themeDependents.erase(GetObjectID());
	}

	m_themeSource = sourceID;
	if (source != nullptr)
	{
		source->m_themeDependents.insert(GetObjectID());
	}

	RefreshThemeBranch();
	return true;
}

void Control::ReleaseThemeDependents()
{
	auto const dependents = m_themeDependents;
	m_themeDependents.clear();
	for (ObjectID id : dependents)
	{
		if (Control* dependent = ObjectDatabase::GetInstance<Control>(id))
		{
			dependent->SetThemeSource(nullptr);
			dependent->OnThemeSourceLost();
		}
	}
}
