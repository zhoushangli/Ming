#include "MingEngine/Scene/GUI/ColorPicker.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include "MingEngine/Core/Image.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace
{
unsigned char Byte(float value)
{
	return static_cast<unsigned char>(std::lround(std::clamp(value, 0.f, 1.f) * 255.f));
}

Color FromHSV(float hue, float saturation, float value, unsigned char alpha)
{
	hue           = std::fmod(hue, 360.f) / 60.f;
	float const c = value * saturation;
	float const x = c * (1.f - std::abs(std::fmod(hue, 2.f) - 1.f));
	float const m = value - c;
	float       r = 0.f;
	float       g = 0.f;
	float       b = 0.f;
	switch (static_cast<int>(hue))
	{
	case 0:
		r = c;
		g = x;
		break;
	case 1:
		r = x;
		g = c;
		break;
	case 2:
		g = c;
		b = x;
		break;
	case 3:
		g = x;
		b = c;
		break;
	case 4:
		r = x;
		b = c;
		break;
	default:
		r = c;
		b = x;
		break;
	}

	return Color(Byte(r + m), Byte(g + m), Byte(b + m), alpha);
}

Rect2 TextureRect(AABB2 const& rect)
{
	return Rect2{ rect.m_mins, rect.GetDimensions() };
}

std::string Hex(Color const& color)
{
	char buffer[10];
	std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X", color.r, color.g, color.b, color.a);
	return buffer;
}

bool ParseHex(std::string text, Color& color)
{
	if (!text.empty() && text.front() == '#')
	{
		text.erase(0, 1);
	}

	if (text.size() != 6 && text.size() != 8)
	{
		return false;
	}

	unsigned value = 0;
	for (char c : text)
	{
		int const digit = c >= '0' && c <= '9'
							  ? c - '0'
							  : (c >= 'a' && c <= 'f' ? c - 'a' + 10 : (c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1));
		if (digit < 0)
		{
			return false;
		}

		value = (value << 4) | static_cast<unsigned>(digit);
	}

	if (text.size() == 6)
	{
		value = (value << 8) | 255;
	}

	color = Color(
		static_cast<unsigned char>(value >> 24),
		static_cast<unsigned char>(value >> 16),
		static_cast<unsigned char>(value >> 8),
		static_cast<unsigned char>(value));
	return true;
}
}

ColorPicker::ColorPicker()
{
	SetFocusMode(FocusMode::All);
	for (int i = 0; i < 4; ++i)
	{
		m_labels[i] = MemNew<Label>();
		m_labels[i]->SetVerticalAlignment(VerticalAlignment::Center);
		AddNode(m_labels[i]);
		m_spins[i] = MemNew<SpinBox>();
		m_spins[i]->SetName(i == 3 ? "Alpha" : String("Channel" + std::to_string(i)));
		m_spins[i]->SetStepButtonsVisible(false);
		m_spins[i]->SetErrorMessagesVisible(false);
		m_spins[i]->Configure(true, "0,255,1");
		m_spins[i]->SetValueChangedCallback(
			[this, i](Variant const& value)
			{
				SetChannel(i, value.Is<int>() ? static_cast<float>(value.As<int>()) : value.As<float>());
			});
		AddNode(m_spins[i]);
	}

	m_rgbButton = MemNew<Button>();
	m_rgbButton->SetText("RGB");
	m_rgbButton->SetToggleMode(true);
	m_rgbButton->SetPressedCallback(
		[this]()
		{
			SetHSVMode(false);
		});
	AddNode(m_rgbButton);
	m_hsvButton = MemNew<Button>();
	m_hsvButton->SetText("HSV");
	m_hsvButton->SetToggleMode(true);
	m_hsvButton->SetPressedCallback(
		[this]()
		{
			SetHSVMode(true);
		});
	AddNode(m_hsvButton);
	m_hexLabel = MemNew<Label>();
	m_hexLabel->SetText("Hex");
	m_hexLabel->SetVerticalAlignment(VerticalAlignment::Center);
	AddNode(m_hexLabel);
	m_hex = MemNew<LineEdit>();
	m_hex->SetName("HexColor");
	m_hex->SetMinimumCharacterWidth(9);
	m_hex->SetEditingEndedCallback(
		[this](String const& text, LineEdit::EditEndReason reason)
		{
			Color color;
			if ((reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost)
				&& ParseHex(text.ToUtf8(), color))
			{
				Publish(color);
			}

			SyncFields();
		});
	AddNode(m_hex);
	m_pickButton = MemNew<Button>();
	m_pickButton->SetText("Pick");
	m_pickButton->SetName("PickScreenColor");
	m_pickButton->SetDisabled(true);
	m_pickButton->SetPressedCallback(
		[this]()
		{
			auto callback = m_pickRequested;
			if (callback)
			{
				callback();
			}
		});
	AddNode(m_pickButton);
	SyncFields();
}

void ColorPicker::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, minimum_width);
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, shape_height);
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, bar_width);
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, row_height);
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, spacing);
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, padding);
	BIND_THEME_ITEM(Theme::DataType::Constant, ColorPicker, checker_size);
	BIND_THEME_ITEM(Theme::DataType::Color, ColorPicker, checker_light);
	BIND_THEME_ITEM(Theme::DataType::Color, ColorPicker, checker_dark);
	BIND_THEME_ITEM(Theme::DataType::Color, ColorPicker, cursor_color);
	BIND_THEME_ITEM(Theme::DataType::StyleBox, ColorPicker, panel);
}

float ColorPicker::FirstRow() const
{
	return static_cast<float>(m_themeCache.padding + m_themeCache.shape_height + m_themeCache.spacing);
}

float ColorPicker::RowPitch() const
{
	return static_cast<float>(m_themeCache.row_height + m_themeCache.spacing);
}

Vector2 ColorPicker::GetInternalMinimumSize() const
{
	return Vector2(
		static_cast<float>(m_themeCache.minimum_width),
		FirstRow() + RowPitch() * 6.f + m_themeCache.row_height + m_themeCache.padding);
}

AABB2 ColorPicker::GetSelectionRect() const
{
	float const p = static_cast<float>(m_themeCache.padding);
	return AABB2(p, p, GetSize().x - p - m_themeCache.spacing - m_themeCache.bar_width, p + m_themeCache.shape_height);
}

AABB2 ColorPicker::GetHueRect() const
{
	AABB2 rect = GetSelectionRect();
	return AABB2(
		rect.m_maxs.x + m_themeCache.spacing,
		rect.m_mins.y,
		GetSize().x - m_themeCache.padding,
		rect.m_maxs.y);
}

AABB2 ColorPicker::GetAlphaRect() const
{
	float const p            = static_cast<float>(m_themeCache.padding);
	float const y            = FirstRow() + RowPitch();
	float const numericWidth = static_cast<float>(m_themeCache.row_height) * 2.75f;
	return AABB2(
		p + m_themeCache.row_height,
		y,
		GetSize().x - p - numericWidth - m_themeCache.spacing,
		y + m_themeCache.row_height);
}

AABB2 ColorPicker::GetChannelRect(int index) const
{
	AABB2       rect  = GetAlphaRect();
	float const shift = RowPitch() * (2.f + static_cast<float>(index));
	rect.Translate(Vector2(0.f, shift));
	return rect;
}

AABB2 ColorPicker::GetOldColorRect() const
{
	float const p = static_cast<float>(m_themeCache.padding);
	return AABB2(p, FirstRow(), GetSize().x * 0.5f, FirstRow() + m_themeCache.row_height);
}

void ColorPicker::LayoutChildren()
{
	if (m_pickButton == nullptr || m_hex == nullptr || m_spins[3] == nullptr)
	{
		return;
	}

	float const p     = static_cast<float>(m_themeCache.padding);
	float const row   = static_cast<float>(m_themeCache.row_height);
	float const gap   = static_cast<float>(m_themeCache.spacing);
	auto        place = [](Control* control, AABB2 const& rect)
	{
		control->SetPosition(rect.m_mins);
		control->UpdateSize(rect.GetDimensions());
	};
	for (int i = 0; i < 4; ++i)
	{
		AABB2 const rect = i == 3 ? GetAlphaRect() : GetChannelRect(i);
		place(m_labels[i], AABB2(p, rect.m_mins.y, rect.m_mins.x, rect.m_maxs.y));
		place(m_spins[i], AABB2(rect.m_maxs.x + gap, rect.m_mins.y, GetSize().x - p, rect.m_maxs.y));
	}

	float const modeY = FirstRow() + RowPitch() * 2.f;
	place(m_rgbButton, AABB2(p, modeY, (GetSize().x - gap) * 0.5f, modeY + row));
	place(m_hsvButton, AABB2((GetSize().x + gap) * 0.5f, modeY, GetSize().x - p, modeY + row));
	float const hexY = FirstRow() + RowPitch() * 6.f;
	place(m_hexLabel, AABB2(p, hexY, p + row * 1.5f, hexY + row));
	place(m_hex, AABB2(p + row * 1.5f, hexY, GetSize().x - p - row * 2.f - gap, hexY + row));
	place(m_pickButton, AABB2(GetSize().x - p - row * 2.f, hexY, GetSize().x - p, hexY + row));
}

void ColorPicker::OnThemeChanged()
{
	m_dragRegion = -1;
	LayoutChildren();
	PropagateMinimumSizeChanged();
	QueueRedraw();
}

void ColorPicker::SetColorNoSignal(Color const& color)
{
	if (m_color == color)
	{
		return;
	}

	m_color = color;
	UpdateHSV();
	SyncFields();
	QueueRedraw();
}

void ColorPicker::SetOldColor(Color const& color)
{
	m_oldColor = color;
	QueueRedraw();
}

void ColorPicker::UpdateHSV()
{
	float const r       = m_color.r / 255.f;
	float const g       = m_color.g / 255.f;
	float const b       = m_color.b / 255.f;
	float const maximum = std::max({ r, g, b });
	float const delta   = maximum - std::min({ r, g, b });
	m_value             = maximum;
	if (maximum > 0.f)
	{
		m_saturation = delta / maximum;
	}

	if (delta > 0.f)
	{
		m_hue = 60.f
				* (maximum == r ? std::fmod((g - b) / delta, 6.f)
								: (maximum == g ? (b - r) / delta + 2.f : (r - g) / delta + 4.f));
		if (m_hue < 0.f)
		{
			m_hue += 360.f;
		}
	}
}

float ColorPicker::GetChannel(int index) const
{
	if (index == 3)
	{
		return static_cast<float>(m_color.a);
	}

	if (m_hsvMode)
	{
		return index == 0 ? m_hue : (index == 1 ? m_saturation : m_value) * 100.f;
	}

	return static_cast<float>(index == 0 ? m_color.r : (index == 1 ? m_color.g : m_color.b));
}

void ColorPicker::SyncFields()
{
	char const* labels[] = { "R", "G", "B", "A", "H", "S", "V" };
	for (int i = 0; i < 4; ++i)
	{
		m_labels[i]->SetText(labels[m_hsvMode && i < 3 ? i + 4 : i]);
		m_spins[i]->SetValueNoSignal(
			m_hsvMode && i < 3 ? Variant(GetChannel(i)) : Variant(static_cast<int>(GetChannel(i))));
	}

	if (!m_hex->IsEditing())
	{
		m_hex->SetText(Hex(m_color));
	}

	m_rgbButton->SetPressedNoSignal(!m_hsvMode);
	m_hsvButton->SetPressedNoSignal(m_hsvMode);
	m_pickButton->SetDisabled(!m_screenReader);
}

void ColorPicker::SetHSVMode(bool enabled)
{
	EndEditing(true);
	m_hsvMode = enabled;
	for (int i = 0; i < 3; ++i)
	{
		m_spins[i]->Configure(!enabled, enabled ? (i == 0 ? "0,360,0.1" : "0,100,0.1") : "0,255,1");
	}

	SyncFields();
	QueueRedraw();
}

void ColorPicker::Publish(Color const& color, bool keepHSV)
{
	bool const changed = m_color != color;
	m_color            = color;
	if (!keepHSV)
	{
		UpdateHSV();
	}

	SyncFields();
	QueueRedraw();
	auto callback = m_changed;
	if (changed && callback)
	{
		callback(color);
	}
}

void ColorPicker::SetChannel(int index, float value)
{
	Color color = m_color;
	if (index == 3)
	{
		color.a = Byte(value / 255.f);
	}
	else if (m_hsvMode)
	{
		if (index == 0)
		{
			m_hue = std::fmod(value, 360.f);
		}
		else if (index == 1)
		{
			m_saturation = value / 100.f;
		}
		else
		{
			m_value = value / 100.f;
		}

		color = FromHSV(m_hue, m_saturation, m_value, m_color.a);
	}
	else
	{
		(index == 0 ? color.r : (index == 1 ? color.g : color.b)) = Byte(value / 255.f);
	}

	Publish(color, m_hsvMode || index == 3);
}

void ColorPicker::EndEditing(bool commit)
{
	LineEdit::EditEndReason const reason =
		commit ? LineEdit::EditEndReason::FocusLost : LineEdit::EditEndReason::Interrupted;
	for (SpinBox* spin : m_spins)
	{
		spin->EndEditing(reason);
	}

	m_hex->EndEditing(reason);
}

bool ColorPicker::ReadScreenColor(Vector2 const& position, Color& color) const
{
	return m_screenReader && m_screenReader(position, color);
}

void ColorPicker::SetScreenColorReader(ScreenColorReader reader)
{
	m_screenReader = std::move(reader);
	SyncFields();
}

bool ColorPicker::HandleKey(InputEvent const& event)
{
	Control* focus = GetViewport() != nullptr ? GetViewport()->GetGuiFocus() : nullptr;
	if (focus == nullptr || (focus != this && !IsAncestorOf(focus)))
	{
		return false;
	}

	if (auto* edit = dynamic_cast<LineEdit*>(focus))
	{
		bool const editing = edit->IsEditing();
		if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_ESCAPE && !editing)
		{
			return false;
		}

		return edit->GuiInput(event);
	}

	return focus != this && focus->GuiInput(event);
}

void ColorPicker::ApplyPointer(Vector2 const& position)
{
	AABB2 const rect =
		m_dragRegion == 0
			? GetSelectionRect()
			: (m_dragRegion == 1 ? GetHueRect()
								 : (m_dragRegion == 2 ? GetAlphaRect() : GetChannelRect(m_dragRegion - 3)));
	Vector2 uv = rect.GetUVForPoint(rect.GetNearestPoint(position));
	uv.x       = std::clamp(uv.x, 0.f, 1.f);
	uv.y       = std::clamp(uv.y, 0.f, 1.f);
	switch (m_dragRegion)
	{
	case 0:
		m_saturation = uv.x;
		m_value      = 1.f - uv.y;
		Publish(FromHSV(m_hue, m_saturation, m_value, m_color.a), true);
		break;
	case 1:
		m_hue = std::min(uv.y * 360.f, 359.999f);
		Publish(FromHSV(m_hue, m_saturation, m_value, m_color.a), true);
		break;
	case 2:
		SetChannel(3, uv.x * 255.f);
		break;
	default:
		SetChannel(m_dragRegion - 3, uv.x * (m_hsvMode ? (m_dragRegion == 3 ? 360.f : 100.f) : 255.f));
		break;
	}
}

bool ColorPicker::GuiInput(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::MouseButton:
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (!event.m_pressed)
			{
				m_dragRegion = -1;
				return true;
			}

			EndEditing(true);
			if (GetOldColorRect().IsPointInside(event.m_position))
			{
				Publish(m_oldColor);
				return true;
			}

			std::array<AABB2, 6> const regions = { GetSelectionRect(), GetHueRect(),      GetAlphaRect(),
												   GetChannelRect(0),  GetChannelRect(1), GetChannelRect(2) };
			for (int i = 0; i < static_cast<int>(regions.size()); ++i)
			{
				if (regions[i].IsPointInside(event.m_position))
				{
					m_dragRegion = i;
					ApplyPointer(event.m_position);
					return true;
				}
			}
		}
		break;
	case InputEventType::MouseMotion:
		if (m_dragRegion >= 0)
		{
			ApplyPointer(event.m_position);
			return true;
		}
		break;
	}

	return false;
}

void ColorPicker::DrawChecker(AABB2 const& rect)
{
	float const cell = static_cast<float>(std::max(1, m_themeCache.checker_size));
	for (float y = rect.m_mins.y; y < rect.m_maxs.y; y += cell)
	{
		for (float x = rect.m_mins.x; x < rect.m_maxs.x; x += cell)
		{
			int const parity =
				static_cast<int>((x - rect.m_mins.x) / cell) + static_cast<int>((y - rect.m_mins.y) / cell);
			DrawRect(
				AABB2(x, y, std::min(x + cell, rect.m_maxs.x), std::min(y + cell, rect.m_maxs.y)),
				parity % 2 == 0 ? m_themeCache.checker_light : m_themeCache.checker_dark);
		}
	}
}

void ColorPicker::DrawMarker(AABB2 const& rect, float value, bool vertical)
{
	Vector2 const point = rect.GetPointAtUV(vertical ? Vector2(0.f, value) : Vector2(value, 0.f));
	float const   width = std::max(1.f, m_themeCache.checker_size / 3.f);
	DrawRect(
		vertical ? AABB2(rect.m_mins.x, point.y - width, rect.m_maxs.x, point.y + width)
				 : AABB2(point.x - width, rect.m_mins.y, point.x + width, rect.m_maxs.y),
		Color::Black);
	DrawRect(
		vertical ? AABB2(rect.m_mins.x, point.y - width * 0.4f, rect.m_maxs.x, point.y + width * 0.4f)
				 : AABB2(point.x - width * 0.4f, rect.m_mins.y, point.x + width * 0.4f, rect.m_maxs.y),
		m_themeCache.cursor_color);
}

void ColorPicker::UpdateTextures()
{
	for (int region = 0; region < 6; ++region)
	{
		std::array<float, 4> key = {};
		if (region == 0)
		{
			key[0] = m_hue;
		}
		else if (region == 2 || (region >= 3 && !m_hsvMode))
		{
			key = { static_cast<float>(m_color.r), static_cast<float>(m_color.g), static_cast<float>(m_color.b), 0.f };
		}
		else if (region >= 3)
		{
			key = { m_hue, m_saturation, m_value, 1.f };
		}

		if (m_gradients[region].IsValid() && key == m_gradientKeys[region])
		{
			continue;
		}

		int const            width  = region == 1 ? 1 : 256;
		int const            height = region == 0 ? 256 : (region == 1 ? 360 : 1);
		std::vector<uint8_t> pixels(static_cast<size_t>(width * height * 4));
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				float const t     = width > 1 ? x / 255.f : y / 359.f;
				Color       color = m_color;
				if (region == 0)
				{
					color = FromHSV(m_hue, t, 1.f - y / 255.f, 255);
				}
				else if (region == 1)
				{
					color = FromHSV(t * 360.f, 1.f, 1.f, 255);
				}
				else if (region == 2)
				{
					color.a = Byte(t);
				}
				else if (m_hsvMode)
				{
					color = FromHSV(
						region == 3 ? t * 360.f : m_hue,
						region == 4 ? t : m_saturation,
						region == 5 ? t : m_value,
						255);
				}
				else
				{
					(region == 3 ? color.r : (region == 4 ? color.g : color.b)) = Byte(t);
					color.a                                                     = 255;
				}

				size_t const offset = static_cast<size_t>((y * width + x) * 4);
				pixels[offset]      = color.r;
				pixels[offset + 1]  = color.g;
				pixels[offset + 2]  = color.b;
				pixels[offset + 3]  = color.a;
			}
		}

		auto image = CreateRef<Image>();
		if (image->LoadFromRGBA8(IntVec2(width, height), std::move(pixels)))
		{
			if (!m_gradients[region].IsValid())
			{
				m_gradients[region]                = CreateRef<TextureResource>();
				m_gradients[region]->m_samplerMode = SamplerMode::BILINEAR_CLAMP;
			}

			m_gradients[region]->m_image = image;
			if (m_gradients[region]->InitGPUResources())
			{
				m_gradientKeys[region] = key;
			}
		}
	}
}

void ColorPicker::DrawContent()
{
	if (m_themeCache.panel.IsValid())
	{
		DrawThemeStyleBox(m_themeCache.panel, GetLocalRect());
	}

	UpdateTextures();
	std::array<AABB2, 6> const regions = { GetSelectionRect(), GetHueRect(),      GetAlphaRect(),
										   GetChannelRect(0),  GetChannelRect(1), GetChannelRect(2) };
	for (int i = 0; i < 6; ++i)
	{
		if (i == 2)
		{
			DrawChecker(regions[i]);
		}

		DrawTextureRect(m_gradients[i], TextureRect(regions[i]));
		if (i > 0)
		{
			float const t = i == 1 ? m_hue / 360.f
								   : (i == 2 ? m_color.a / 255.f
											 : GetChannel(i - 3) / (m_hsvMode ? (i == 3 ? 360.f : 100.f) : 255.f));
			DrawMarker(regions[i], t, i == 1);
		}
	}

	Vector2 const selected = GetSelectionRect().GetPointAtUV(Vector2(m_saturation, 1.f - m_value));
	float const   radius   = static_cast<float>(std::max(2, m_themeCache.checker_size));
	DrawRect(AABB2(selected - Vector2(radius, radius), selected + Vector2(radius, radius)), Color::Black);
	DrawRect(
		AABB2(selected - Vector2(radius * 0.65f, radius * 0.65f), selected + Vector2(radius * 0.65f, radius * 0.65f)),
		m_themeCache.cursor_color);
	DrawRect(
		AABB2(selected - Vector2(radius * 0.35f, radius * 0.35f), selected + Vector2(radius * 0.35f, radius * 0.35f)),
		FromHSV(m_hue, m_saturation, m_value, 255));
	AABB2 old = GetOldColorRect();
	AABB2 current(old.m_maxs.x, old.m_mins.y, GetSize().x - m_themeCache.padding, old.m_maxs.y);
	DrawChecker(old);
	DrawRect(old, m_oldColor);
	DrawChecker(current);
	DrawRect(current, m_color);
}

void ColorPicker::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
		DrawContent();
		break;
	case Notification_Resized:
		LayoutChildren();
		break;
	case Notification_GuiInputCancelled:
	case Notification_ExitTree:
		m_dragRegion = -1;
		EndEditing(false);
		break;
	}
}
