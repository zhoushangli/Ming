#include "MingEngine/Scene/GUI/ProgressBar.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include <algorithm>
#include <cmath>

ProgressBar::ProgressBar()
{
	SetMouseFilter(MouseFilter::Ignore);
}

void ProgressBar::BindMethods()
{
	ClassDatabase::BindMethod("GetMinValue", &ProgressBar::GetMinValue, {});
	ClassDatabase::BindMethod("GetMaxValue", &ProgressBar::GetMaxValue, {});
	ClassDatabase::BindMethod("GetValue", &ProgressBar::GetValue, {});
	ClassDatabase::BindMethod("IsIndeterminate", &ProgressBar::IsIndeterminate, {});
	ClassDatabase::BindMethod("SetMinValue", &ProgressBar::SetMinValue, { "value" });
	ClassDatabase::BindMethod("SetMaxValue", &ProgressBar::SetMaxValue, { "value" });
	ClassDatabase::BindMethod("SetValue", &ProgressBar::SetValue, { "value" });
	ClassDatabase::BindMethod("SetIndeterminate", &ProgressBar::SetIndeterminate, { "enabled" });

	ADD_PROPERTY(PropertyInfo(Variant::Type::Float, "min_value", PropertyInfo::Hint::None, "",
		PropertyInfo::UsageFlags::Default), "SetMinValue", "GetMinValue");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Float, "max_value", PropertyInfo::Hint::None, "",
		PropertyInfo::UsageFlags::Default), "SetMaxValue", "GetMaxValue");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Float, "value", PropertyInfo::Hint::None, "",
		PropertyInfo::UsageFlags::Default), "SetValue", "GetValue");
	ADD_PROPERTY(PropertyInfo(Variant::Type::Bool, "indeterminate", PropertyInfo::Hint::None, "",
		PropertyInfo::UsageFlags::Default), "SetIndeterminate", "IsIndeterminate");

	BIND_THEME_ITEM(Theme::DataType::StyleBox, ProgressBar, background);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, ProgressBar, fill);
	BIND_THEME_ITEM(Theme::DataType::Constant, ProgressBar, minimum_height);
}

Vector2 ProgressBar::GetInternalMinimumSize() const
{
	Vector2 size = m_themeCache.background.IsValid() ? m_themeCache.background->GetMinimumSize() : Vector2::Zero;
	size.y = std::max(size.y, static_cast<float>(m_themeCache.minimum_height));
	return size;
}

void ProgressBar::SetMinValue(float value)
{
	if (!std::isfinite(value) || m_minValue == value)
	{
		return;
	}

	m_minValue = value;
	SetValue(m_value);
	QueueRedraw();
}

void ProgressBar::SetMaxValue(float value)
{
	if (!std::isfinite(value) || m_maxValue == value)
	{
		return;
	}

	m_maxValue = value;
	SetValue(m_value);
	QueueRedraw();
}

void ProgressBar::SetValue(float value)
{
	if (!std::isfinite(value))
	{
		return;
	}

	value = std::clamp(value, m_minValue, std::max(m_minValue, m_maxValue));
	if (m_value != value)
	{
		m_value = value;
		QueueRedraw();
	}
}

void ProgressBar::SetIndeterminate(bool enabled)
{
	if (m_indeterminate == enabled)
	{
		return;
	}

	m_indeterminate = enabled;
	m_phase = 0.f;
	SetProcess(enabled);
	QueueRedraw();
}

void ProgressBar::OnProcess(float deltaSeconds)
{
	if (m_indeterminate && IsVisibleInHierarchy())
	{
		m_phase = std::fmod(m_phase + std::max(0.f, deltaSeconds) * 0.8f, 1.f);
		QueueRedraw();
	}
}

void ProgressBar::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
	{
		DrawThemeStyleBox(m_themeCache.background, GetLocalRect());
		AABB2 fill = m_themeCache.background.IsValid()
			? m_themeCache.background->GetContentRect(GetLocalRect()) : GetLocalRect();
		float const width = std::max(0.f, fill.GetWidth());
		if (m_indeterminate)
		{
			float const start = m_phase * 1.3f - 0.3f;
			fill.m_maxs.x = fill.m_mins.x + width * std::clamp(start + 0.3f, 0.f, 1.f);
			fill.m_mins.x += width * std::clamp(start, 0.f, 1.f);
		}
		else
		{
			double const range = static_cast<double>(m_maxValue) - m_minValue;
			float const ratio = range > 0.0
				? static_cast<float>(std::clamp((static_cast<double>(m_value) - m_minValue) / range, 0.0, 1.0)) : 0.f;
			fill.m_maxs.x = fill.m_mins.x + width * ratio;
		}

		if (fill.GetWidth() > 0.f && fill.GetHeight() > 0.f)
		{
			DrawThemeStyleBox(m_themeCache.fill, fill);
		}
		break;
	}
	}
}
