#include "MingEngine/Scene/Resource/ThemeStyleBox.hpp"

#include "MingEngine/Scene/GUI/Control.hpp"

#include <algorithm>
#include <cmath>

float ThemeStyleBox::GetBorderWidth(Side side) const
{
	int const index = static_cast<int>(side);
	return index >= 0 && index < 4 ? m_borderWidths[index] : 0.f;
}

float ThemeStyleBox::GetContentMargin(Side side) const
{
	int const index = static_cast<int>(side);
	return index >= 0 && index < 4 ? m_contentMargins[index] : 0.f;
}

Vector2 ThemeStyleBox::GetMinimumSize() const
{
	return Vector2(m_contentMargins[0] + m_contentMargins[2], m_contentMargins[1] + m_contentMargins[3]);
}

AABB2 ThemeStyleBox::GetContentRect(AABB2 const& rect) const
{
	Vector2 const start(
		std::min(rect.m_maxs.x, rect.m_mins.x + m_contentMargins[0]),
		std::min(rect.m_maxs.y, rect.m_mins.y + m_contentMargins[1]));
	return AABB2(
		start,
		Vector2(
			std::max(start.x, rect.m_maxs.x - m_contentMargins[2]),
			std::max(start.y, rect.m_maxs.y - m_contentMargins[3])));
}

void ThemeStyleBox::SetBackgroundColor(Color const& color)
{
	if (m_backgroundColor != color)
	{
		m_backgroundColor = color;
		EmitChanged();
	}
}

void ThemeStyleBox::SetBorderColor(Color const& color)
{
	if (m_borderColor != color)
	{
		m_borderColor = color;
		EmitChanged();
	}
}

void ThemeStyleBox::SetBorderWidth(Side side, float width)
{
	int const index = static_cast<int>(side);
	if (index < 0 || index >= 4 || !std::isfinite(width))
	{
		return;
	}

	width = std::max(0.f, width);
	if (m_borderWidths[index] != width)
	{
		m_borderWidths[index] = width;
		EmitChanged();
	}
}

void ThemeStyleBox::SetContentMargin(Side side, float margin)
{
	int const index = static_cast<int>(side);
	if (index < 0 || index >= 4 || !std::isfinite(margin))
	{
		return;
	}

	margin = std::max(0.f, margin);
	if (m_contentMargins[index] != margin)
	{
		m_contentMargins[index] = margin;
		EmitChanged();
	}
}

void ThemeStyleBox::AddUser(ObjectID user)
{
	++m_users[user];
}

void ThemeStyleBox::RemoveUser(ObjectID user)
{
	auto found = m_users.find(user);
	if (found != m_users.end() && --found->second == 0)
	{
		m_users.erase(found);
	}
}

void ThemeStyleBox::EmitChanged()
{
	Ref<ThemeStyleBox> keepAlive(GetRefCount() > 0 ? this : nullptr);
	auto const         users = m_users;
	for (auto const& entry : users)
	{
		if (m_users.find(entry.first) == m_users.end())
		{
			continue;
		}

		Object* user = ObjectDatabase::GetInstance(entry.first);
		if (user == nullptr || user->IsPreDeleting())
		{
			m_users.erase(entry.first);
			continue;
		}

		if (auto* theme = dynamic_cast<Theme*>(user))
		{
			theme->EmitChanged();
		}
		else if (auto* control = dynamic_cast<Control*>(user))
		{
			control->RefreshTheme();
		}
	}
}

bool ThemeStyleBox::CopyFrom(Resource&& other)
{
	auto* box = dynamic_cast<ThemeStyleBox*>(&other);
	if (box == nullptr || box == this)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));
	m_backgroundColor = box->m_backgroundColor;
	m_borderColor     = box->m_borderColor;
	std::copy(std::begin(box->m_borderWidths), std::end(box->m_borderWidths), m_borderWidths);
	std::copy(std::begin(box->m_contentMargins), std::end(box->m_contentMargins), m_contentMargins);
	box->m_backgroundColor = Color::Transparent;
	box->m_borderColor     = Color::Transparent;
	std::fill(std::begin(box->m_borderWidths), std::end(box->m_borderWidths), 0.f);
	std::fill(std::begin(box->m_contentMargins), std::end(box->m_contentMargins), 0.f);
	EmitChanged();
	box->EmitChanged();
	return true;
}
