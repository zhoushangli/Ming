#include "MingEngine/Scene/GUI/Control.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include <algorithm>

#include <array>
#include <cmath>

namespace
{
constexpr int roundedCornerSegments = 8;
constexpr int roundedContourSize    = 4 * (roundedCornerSegments + 1);
using StyleContour                  = std::array<Vertex, roundedContourSize>;

Color MixStyleColor(Color a, Color b, float t)
{
	auto channel = [t](unsigned char x, unsigned char y)
	{
		return static_cast<unsigned char>(std::clamp(std::round(x + (y - x) * t), 0.f, 255.f));
	};
	return Color(channel(a.r, b.r), channel(a.g, b.g), channel(a.b, b.b), channel(a.a, b.a));
}

std::vector<Vertex> BuildRoundedStyleVertices(ThemeStyleBox const& box, AABB2 const& rect)
{
	float const width  = rect.GetWidth();
	float const height = rect.GetHeight();
	if (width <= 0.f || height <= 0.f)
	{
		return {};
	}

	// 1) Fit the requested radii and border widths to the available rectangle.
	float radius[4];
	float border[4];
	for (int i = 0; i < 4; ++i)
	{
		radius[i] = box.GetCornerRadius(static_cast<ThemeStyleBox::Corner>(i));
		border[i] = std::min(i % 2 == 0 ? width : height, box.GetBorderWidth(static_cast<ThemeStyleBox::Side>(i)));
	}

	double factor = 1.0;
	auto   fit    = [&factor](float size, float a, float b)
	{
		double const sum = static_cast<double>(a) + b;
		if (sum > size)
		{
			factor = std::min(factor, size / sum);
		}
	};
	fit(width, radius[0], radius[1]);
	fit(width, radius[3], radius[2]);
	fit(height, radius[0], radius[3]);
	fit(height, radius[1], radius[2]);
	for (float& value : radius)
	{
		value = static_cast<float>(value * factor);
	}

	for (int axis = 0; axis < 2; ++axis)
	{
		float const size = (axis == 0 ? width : height) - std::min(1.f, std::min(width, height) * 0.5f);
		float const sum  = border[axis] + border[axis + 2];
		if (sum > size)
		{
			border[axis] *= size / sum;
			border[axis + 2] *= size / sum;
		}
	}

	Color const fill = box.GetBackgroundColor();
	Color const edge = box.GetBorderColor();
	if (fill.a == 0 && edge.a == 0)
	{
		return {};
	}

	float const halfAA  = std::min(0.5f, std::min(width, height) * 0.25f);
	auto        contour = [&](std::array<float, 4> inset, bool edgeColor, bool transparent)
	{
		StyleContour result;
		float const  x0 = rect.m_mins.x + inset[0];
		float const  y0 = rect.m_mins.y + inset[1];
		float const  x1 = std::max(x0, rect.m_maxs.x - inset[2]);
		float const  y1 = std::max(y0, rect.m_maxs.y - inset[3]);
		float        rx[4];
		float        ry[4];
		for (int corner = 0; corner < 4; ++corner)
		{
			int const horizontal = corner == 0 || corner == 3 ? 0 : 2;
			int const vertical   = corner < 2 ? 1 : 3;
			rx[corner]           = radius[corner] > 0.f ? std::max(0.f, radius[corner] - inset[horizontal]) : 0.f;
			ry[corner]           = radius[corner] > 0.f ? std::max(0.f, radius[corner] - inset[vertical]) : 0.f;
		}

		factor = 1.0;
		fit(x1 - x0, rx[0], rx[1]);
		fit(x1 - x0, rx[3], rx[2]);
		fit(y1 - y0, ry[0], ry[3]);
		fit(y1 - y0, ry[1], ry[2]);
		for (int corner = 0; corner < 4; ++corner)
		{
			rx[corner] *= static_cast<float>(factor);
			ry[corner] *= static_cast<float>(factor);
			int const   horizontal = corner == 0 || corner == 3 ? 0 : 2;
			int const   vertical   = corner < 2 ? 1 : 3;
			float const cx         = horizontal == 0 ? x0 + rx[corner] : x1 - rx[corner];
			float const cy         = vertical == 1 ? y0 + ry[corner] : y1 - ry[corner];
			for (int step = 0; step <= roundedCornerSegments; ++step)
			{
				float const angle =
					(corner + step / static_cast<float>(roundedCornerSegments)) * 1.57079632679f + 3.14159265359f;
				float const   cosine = std::cos(angle);
				float const   sine   = std::sin(angle);
				Vector3 const position(cx + rx[corner] * cosine, cy + ry[corner] * sine, 0.f);
				float const   horizontalWeight = std::abs(cosine);
				float const   verticalWeight   = std::abs(sine);
				float const   edgeWeight       = ((border[horizontal] > 0.f ? horizontalWeight : 0.f)
												  + (border[vertical] > 0.f ? verticalWeight : 0.f))
												 / (horizontalWeight + verticalWeight);
				Color         color            = edgeColor ? MixStyleColor(fill, edge, edgeWeight) : fill;
				if (transparent)
				{
					color.a = 0;
				}

				result[corner * (roundedCornerSegments + 1) + step] = Vertex(position, color, Vector2::Zero);
			}
		}
		return result;
	};

	// 2) Generate adjacent strips so translucent fill and borders never overlap.
	std::array<float, 4> outerInset;
	std::array<float, 4> innerEdgeInset;
	std::array<float, 4> innerFillInset;
	for (int side = 0; side < 4; ++side)
	{
		outerInset[side]     = halfAA;
		innerEdgeInset[side] = std::max(halfAA, border[side] - halfAA);
		innerFillInset[side] = border[side] > 0.f ? border[side] + halfAA : halfAA;
	}

	// Collapse opposing insets together when a border consumes the entire control.
	// e.g. A 20-pixel border on a 6-pixel button must not invert its inner contour.
	for (int axis = 0; axis < 2; ++axis)
	{
		float const size = axis == 0 ? width : height;
		for (auto* inset : { &innerEdgeInset, &innerFillInset })
		{
			float const sum = (*inset)[axis] + (*inset)[axis + 2];
			if (sum > size)
			{
				(*inset)[axis] *= size / sum;
				(*inset)[axis + 2] *= size / sum;
			}
		}
	}

	if (innerFillInset[0] + innerFillInset[2] >= width - 0.00001f
		|| innerFillInset[1] + innerFillInset[3] >= height - 0.00001f)
	{
		innerEdgeInset = { width * 0.5f, height * 0.5f, width * 0.5f, height * 0.5f };
		innerFillInset = innerEdgeInset;
	}

	auto const          outer     = contour(outerInset, true, false);
	auto const          fringe    = contour({ -halfAA, -halfAA, -halfAA, -halfAA }, true, true);
	auto const          innerEdge = contour(innerEdgeInset, true, false);
	auto const          innerFill = contour(innerFillInset, false, false);
	std::vector<Vertex> vertices;
	vertices.reserve(roundedContourSize * 21);
	auto triangle = [&vertices](Vertex const& a, Vertex const& b, Vertex const& c)
	{
		float const area = (b.m_position.x - a.m_position.x) * (c.m_position.y - a.m_position.y)
						   - (b.m_position.y - a.m_position.y) * (c.m_position.x - a.m_position.x);
		if (std::abs(area) > 0.000001f && (a.m_color.a != 0 || b.m_color.a != 0 || c.m_color.a != 0))
		{
			vertices.insert(vertices.end(), { a, b, c });
		}
	};
	auto ring = [&triangle](StyleContour const& a, StyleContour const& b)
	{
		for (int i = 0; i < roundedContourSize; ++i)
		{
			int const next = (i + 1) % roundedContourSize;
			triangle(a[i], a[next], b[i]);
			triangle(a[next], b[next], b[i]);
		}
	};
	ring(fringe, outer);
	ring(outer, innerEdge);
	ring(innerEdge, innerFill);
	Vertex const center(
		Vector3(
			(rect.m_mins.x + innerFillInset[0] + rect.m_maxs.x - innerFillInset[2]) * 0.5f,
			(rect.m_mins.y + innerFillInset[1] + rect.m_maxs.y - innerFillInset[3]) * 0.5f,
			0.f),
		fill);
	for (int i = 0; i < roundedContourSize; ++i)
	{
		triangle(center, innerFill[i], innerFill[(i + 1) % roundedContourSize]);
	}
	return vertices;
}
}

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
	std::function<bool(Vector2 const&, GuiDragData&)>       source,
	std::function<bool(Vector2 const&, GuiDragData const&)> check,
	std::function<void(Vector2 const&, GuiDragData const&)> drop)
{
	m_dragSource = std::move(source);
	m_dropCheck  = std::move(check);
	m_drop       = std::move(drop);
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

	Node const*        contextNode = this;
	std::set<ObjectID> visited;
	for (Control const* current = this; current != nullptr && visited.insert(current->GetObjectID()).second;
		 current                = current->GetThemeParent())
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
		auto callbacks = m_themeChangedCallbacks;
		for (auto const& callback : callbacks)
		{
			callback();
		}
		PropagateMinimumSizeChanged();
		QueueRedraw();
	}

	m_refreshingTheme     = false;
	auto const dependents = m_themeDependents;
	for (ObjectID id : dependents)
	{
		if (Control* dependent = ObjectDatabase::GetInstance<Control>(id))
		{
			dependent->RefreshThemeBranch();
		}
	}
}

void Control::AddThemeChangedCallback(std::function<void()> callback)
{
	if (callback)
	{
		m_themeChangedCallbacks.push_back(callback);
		callback();
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

	bool rounded = false;
	for (int i = 0; i < 4; ++i)
	{
		rounded = rounded || box->GetCornerRadius(static_cast<ThemeStyleBox::Corner>(i)) > 0.f;
	}

	if (rounded)
	{
		DrawTriangles(BuildRoundedStyleVertices(*box.Get(), rect));
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
