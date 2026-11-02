#include "MingEngine/Editor/UI/ViewportPanel.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/TextureRect.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cmath>


void ViewportPanel::BindMethods()
{
	BIND_THEME_ITEM(Theme::DataType::Constant, ViewportPanel, ruler_width);
	BIND_THEME_ITEM(Theme::DataType::Font, ViewportPanel, ruler_font);
	BIND_THEME_ITEM(Theme::DataType::FontSize, ViewportPanel, ruler_font_size);
	BIND_THEME_ITEM(Theme::DataType::Color, ViewportPanel, ruler_background);
	BIND_THEME_ITEM(Theme::DataType::Color, ViewportPanel, ruler_tick_color);
	BIND_THEME_ITEM(Theme::DataType::Color, ViewportPanel, ruler_major_color);
	BIND_THEME_ITEM(Theme::DataType::Color, ViewportPanel, ruler_font_color);
}

ViewportPanel::ViewportPanel() : EditorPanel("Viewport")
{

}

void ViewportPanel::OnInitialize()
{
	SetCustomMinimumSize(Vector2(static_cast<float>(GetThemeConstant("viewport_min_width", "Editor")), static_cast<float>(GetThemeConstant("viewport_min_height", "Editor"))));
	auto* layout = MemNew<VBoxContainer>();
	AddNode(layout);
	layout->UpdateSize(GetSize());
	auto* header = MemNew<PanelContainer>();
	header->SetThemeTypeVariation("EditorViewportHeader");
	header->SetCustomMinimumSize(Vector2(0.f, static_cast<float>(GetThemeConstant("viewport_header_height", "Editor"))));
	layout->AddNode(header);
	auto* headerRow = MemNew<HBoxContainer>();
	header->AddNode(headerRow);
	auto* tab = MemNew<PanelContainer>();
	tab->SetThemeTypeVariation("EditorSceneTab");
	headerRow->AddNode(tab);
	auto* tabRow = MemNew<HBoxContainer>();
	tabRow->SetSeparation(static_cast<float>(GetThemeConstant("status_spacing", "Editor")) * 0.5f);
	tab->AddNode(tabRow);
	auto* sceneIcon = MemNew<TextureRect>();
	sceneIcon->SetTexture(GetThemeIcon("PackedScene", "EditorIcons"));
	sceneIcon->SetTint(GetThemeColor("tn", "EditorIcons"));
	sceneIcon->SetExpandMode(TextureRect::ExpandMode::IgnoreSize);
	sceneIcon->SetStretchMode(TextureRect::StretchMode::KeepAspectCentered);
	sceneIcon->SetCustomMinimumSize(Vector2(static_cast<float>(GetThemeConstant("icon_size", "Button")), static_cast<float>(GetThemeConstant("icon_size", "Button"))));
	tabRow->AddNode(sceneIcon);
	m_sceneTitle = EditorPopupUtils::AddLabel(tabRow, "");
	m_sceneTitle->SetClipText(true);
	m_sceneTitle->SetCustomMinimumSize(Vector2(static_cast<float>(GetThemeConstant("scene_title_width", "Editor")), 0.f));
	m_sceneTitle->SetHorizontalSizeFlags(Size_ExpandFill);
	m_sceneTitle->SetVerticalAlignment(VerticalAlignment::Center);
	auto addTool = [this](Control* parent, char const* name)
	{
		Button* button = EditorPopupUtils::AddButton(parent, "", {});
		button->SetName(name);
		button->SetIcon(GetThemeIcon(name, "EditorIcons"));
		button->SetThemeTypeVariation("EditorToolbarButton");
		button->SetDisabled(true);
		return button;
	};
	addTool(tabRow, "Close")->SetThemeTypeVariation("EditorPanelAction");
	addTool(headerRow, "Add")->SetThemeTypeVariation("EditorPanelAction");
	auto* headerSpacer = MemNew<Control>();
	headerSpacer->SetHorizontalSizeFlags(Size_ExpandFill);
	headerRow->AddNode(headerSpacer);
	addTool(headerRow, "More")->SetThemeTypeVariation("EditorPanelAction");
	auto* toolbar = MemNew<HBoxContainer>();
	toolbar->SetCustomMinimumSize(Vector2(0.f, static_cast<float>(GetThemeConstant("viewport_toolbar_height", "Editor"))));
	toolbar->SetSeparation(static_cast<float>(GetThemeConstant("panel_margin", "Editor")) * 0.5f);
	layout->AddNode(toolbar);
	for (char const* icon : { "ToolSelect", "ToolMove", "ToolRotate", "ToolScale" })
	{
		addTool(toolbar, icon);
	}

	auto* divider = MemNew<ColorRect>();
	divider->SetColor(GetThemeColor("separator_color", "MenuBar"));
	divider->SetCustomMinimumSize(Vector2(1.f, static_cast<float>(GetThemeConstant("separator_height", "Editor"))));
	divider->SetVerticalSizeFlags(Size_ShrinkCenter);
	toolbar->AddNode(divider);
	addTool(toolbar, "Grid");
	addTool(toolbar, "Snap");
	m_local = EditorPopupUtils::AddLabel(toolbar, "Local", "EditorStatusLabel");
	auto* spacer = MemNew<Control>();
	spacer->SetHorizontalSizeFlags(Size_ExpandFill);
	toolbar->AddNode(spacer);
	m_zoomOut = addTool(toolbar, "ZoomOut");
	m_zoom = EditorPopupUtils::AddLabel(toolbar, "100%", "EditorStatusLabel");
	m_zoom->SetName("ZoomReadout");
	m_zoom->SetVerticalAlignment(VerticalAlignment::Center);
	m_zoomIn = addTool(toolbar, "ZoomIn");
	addTool(toolbar, "CenterView");
	m_frame = MemNew<MarginContainer>();
	m_frame->SetMargins(static_cast<float>(m_themeCache.ruler_width), static_cast<float>(m_themeCache.ruler_width), 0.f, 0.f);
	m_frame->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(m_frame);
	m_view = MemNew<SubViewportContainer>();
	m_view->SetVerticalSizeFlags(Size_ExpandFill);
	m_view->SetInputCallback([this](InputEvent const& event) { return HandleSceneInput(event); });
	m_frame->AddNode(m_view);
}

void ViewportPanel::OnRender(EditorUIContext& context)
{
	std::string const name = EditorNode::Get()->GetCurrentSceneName();
	m_sceneTitle->SetText((name.empty() ? "Untitled" : name) + (EditorNode::Get()->IsSceneDirty() ? " *" : ""));
	bool const compact = GetSize().x < 525.f;
	m_local->SetVisible(!compact);
	m_zoomOut->SetVisible(!compact);
	m_zoomIn->SetVisible(!compact);
	context.m_editorUI->SetViewportRect(m_view->GetGlobalPosition(), m_view->GetSize());
	Vector2 const origin = m_frame->GetGlobalPosition() - GetGlobalPosition();
	AABB2 const rect(origin, origin + m_frame->GetSize());
	IntVec2 const resolution = m_view->GetSubViewport()->GetOutputResolution();
	if (!(rect == m_rulerRect) || resolution != m_rulerResolution)
	{
		m_rulerRect = rect;
		m_rulerResolution = resolution;
		QueueRedraw();
	}
}

void ViewportPanel::OnThemeChanged()
{
	m_rulerText.clear();
	QueueRedraw();
}

void ViewportPanel::OnNotification(int notification)
{
	switch (notification)
	{
	case Notification_Draw:
		DrawRulers();
		break;
	}
}

Ref<TextureResource> ViewportPanel::GetRulerText(std::string const& text, bool vertical)
{
	auto found = m_rulerText.find(text);
	if (found != m_rulerText.end())
	{
		return found->second[vertical ? 1 : 0];
	}

	if (!m_themeCache.ruler_font.IsValid() || g_engine == nullptr || g_engine->m_textServer == nullptr)
	{
		return {};
	}

	// 1) Assemble the cached glyph coverage into a horizontal RGBA label
	TextServer* server = g_engine->m_textServer;
	RID const font = m_themeCache.ruler_font->GetFontRID();
	int const fontSize = std::max(1, m_themeCache.ruler_font_size);
	TextLine const line = server->GetTextLine(font, String(text), static_cast<float>(fontSize));
	int const width = std::max(1, static_cast<int>(std::ceil(line.m_width)));
	int const height = std::max(1, static_cast<int>(std::ceil(line.m_config.m_ascent + std::abs(line.m_config.m_descent))));
	std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4, 0);
	Color const color = m_themeCache.ruler_font_color;
	for (TextLineGlyph const& glyph : line.m_glyphs)
	{
		GlyphTexture const* cached = server->GetOrCreateGlyphTexture(font, glyph.m_glyphIndex, fontSize);
		if (cached == nullptr)
		{
			continue;
		}

		GlyphImage const& bitmap = cached->m_bitmap;
		int const left = static_cast<int>(std::round(glyph.m_posX + bitmap.m_offsetX));
		int const top = static_cast<int>(std::round(line.m_config.m_ascent + bitmap.m_offsetY));
		for (int y = 0; y < bitmap.m_height; ++y)
		{
			for (int x = 0; x < bitmap.m_width; ++x)
			{
				int const targetX = left + x;
				int const targetY = top + y;
				if (targetX < 0 || targetX >= width || targetY < 0 || targetY >= height)
				{
					continue;
				}

				size_t const offset = (static_cast<size_t>(targetY) * width + targetX) * 4;
				pixels[offset] = color.r;
				pixels[offset + 1] = color.g;
				pixels[offset + 2] = color.b;
				uint8_t const alpha = static_cast<uint8_t>(bitmap.m_pixels[static_cast<size_t>(y) * bitmap.m_width + x] * color.a / 255);
				pixels[offset + 3] = std::max(pixels[offset + 3], alpha);
			}
		}
	}

	// 2) Rotate a second copy counterclockwise
	std::vector<uint8_t> rotated(pixels.size(), 0);
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			size_t const source = (static_cast<size_t>(y) * width + x) * 4;
			size_t const target = (static_cast<size_t>(width - 1 - x) * height + y) * 4;
			std::copy_n(pixels.data() + source, 4, rotated.data() + target);
		}
	}

	// 3) Upload both orientations once and retain them until the Theme changes
	std::array<Ref<TextureResource>, 2> textures;
	for (int orientation = 0; orientation < 2; ++orientation)
	{
		auto image = CreateRef<Image>();
		IntVec2 const size = orientation == 0 ? IntVec2(width, height) : IntVec2(height, width);
		if (!image->LoadFromRGBA8(size, orientation == 0 ? std::move(pixels) : std::move(rotated)))
		{
			return {};
		}

		auto texture = CreateRef<TextureResource>();
		texture->m_image = image;
		if (!texture->InitGPUResources())
		{
			return {};
		}

		textures[orientation] = texture;
	}

	m_rulerText.emplace(text, textures);
	return textures[vertical ? 1 : 0];
}

void ViewportPanel::DrawRulers()
{
	if (m_frame == nullptr || m_rulerResolution.x < 1 || m_rulerResolution.y < 1)
	{
		return;
	}

	Vector2 const corner(std::round(m_rulerRect.m_mins.x), std::round(m_rulerRect.m_mins.y));
	Vector2 const origin = corner + Vector2(static_cast<float>(m_themeCache.ruler_width), static_cast<float>(m_themeCache.ruler_width));
	Vector2 const end(std::round(m_rulerRect.m_maxs.x), std::round(m_rulerRect.m_maxs.y));
	DrawRect(AABB2(corner, Vector2(end.x, origin.y)), m_themeCache.ruler_background);
	DrawRect(AABB2(Vector2(corner.x, origin.y), Vector2(origin.x, end.y)), m_themeCache.ruler_background);
	auto drawText = [this](Ref<TextureResource> const& texture, Vector2 position)
	{
		if (texture.IsValid())
		{
			Rect2 rect;
			rect.m_position = Vector2(std::round(position.x), std::round(position.y));
			rect.m_size = Vector2(texture->GetDimensions());
			DrawTextureRect(texture, rect);
		}
	};
	Ref<TextureResource> unit = GetRulerText("px", false);
	if (unit.IsValid())
	{
		drawText(unit, corner + (Vector2(static_cast<float>(m_themeCache.ruler_width), static_cast<float>(m_themeCache.ruler_width)) - Vector2(unit->GetDimensions())) * 0.5f);
	}

	for (int axis = 0; axis < 2; ++axis)
	{
		int const extent = axis == 0 ? m_rulerResolution.x : m_rulerResolution.y;
		float const start = axis == 0 ? origin.x : origin.y;
		float const finish = axis == 0 ? end.x : end.y;
		for (int pixel = 0; pixel < extent; pixel += 10)
		{
			float const at = std::round(start + pixel * (finish - start) / extent);
			bool const major = pixel % 100 == 0;
			float const length = major ? 11.f : (pixel % 50 == 0 ? 8.f : 5.f);
			AABB2 const tick = axis == 0 ? AABB2(at, origin.y - length, at + 1.f, origin.y)
				: AABB2(origin.x - length, at, origin.x, at + 1.f);
			DrawRect(tick, major ? m_themeCache.ruler_major_color : m_themeCache.ruler_tick_color);
			if (!major)
			{
				continue;
			}

			Ref<TextureResource> text = GetRulerText(std::to_string(pixel), axis == 1);
			if (!text.IsValid())
			{
				continue;
			}

			IntVec2 const size = text->GetDimensions();
			float const textLength = static_cast<float>(axis == 0 ? size.x : size.y);
			if (at + 3.f + textLength <= finish)
			{
				drawText(text, axis == 0 ? Vector2(at + 3.f, corner.y + 1.f) : Vector2(corner.x + 1.f, at + 3.f));
			}
		}
	}
}

bool ViewportPanel::HandleSceneInput(InputEvent const& event)
{
	EditorNode* owner = EditorNode::Get();
	switch (event.m_type)
	{
	case InputEventType::WindowFocus:
		if (!event.m_focused)
		{
			m_navigating = false;
		}
		break;
	case InputEventType::MouseMotion:
		owner->OnMouseMove(event.m_position + m_view->GetGlobalPosition(), event.m_relative);
		break;
	case InputEventType::MouseButton:
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_navigating = event.m_pressed && m_view->IsPointInside(event.m_position);
			if (m_navigating)
			{
				m_view->GrabFocus();
			}
		}
		else if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			if (event.m_pressed)
			{
				owner->OnMouseDown(ToKeyCode(KeyCode::LeftMouse), event.m_position + m_view->GetGlobalPosition());
			}
			else
			{
				owner->OnMouseUp(ToKeyCode(KeyCode::LeftMouse), event.m_position + m_view->GetGlobalPosition());
			}
		}
		break;
	}

	return event.m_type != InputEventType::Key;
}
