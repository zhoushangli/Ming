#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"

#include <array>
#include <map>

class Label;
class MarginContainer;
class Button;

class ViewportPanel final : public EditorPanel
{
	MCLASS(ViewportPanel, EditorPanel);

private:
	struct ThemeCache
	{
		Ref<FontResource> ruler_font;
		int ruler_font_size = 13;
		int ruler_width = 23;
		Color ruler_background;
		Color ruler_tick_color;
		Color ruler_major_color;
		Color ruler_font_color;
	};

public:
	ViewportPanel();
	SubViewportContainer* GetSceneView() const { return m_view; }
	bool IsNavigating() const { return m_navigating; }
	void CancelNavigation() { m_navigating = false; }

protected:
	static void BindMethods();

	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;
	void OnThemeChanged() override;
	void OnNotification(int notification);

private:
	void DrawRulers();
	Ref<TextureResource> GetRulerText(std::string const& text, bool vertical);
	bool HandleSceneInput(InputEvent const& event);

private:
	Label* m_sceneTitle = nullptr;
	Label* m_local = nullptr;
	Label* m_zoom = nullptr;
	Button* m_zoomOut = nullptr;
	Button* m_zoomIn = nullptr;
	MarginContainer* m_frame = nullptr;
	SubViewportContainer* m_view = nullptr;
	bool m_navigating = false;
	AABB2 m_rulerRect;
	IntVec2 m_rulerResolution;
	std::map<std::string, std::array<Ref<TextureResource>, 2>> m_rulerText;
	ThemeCache m_themeCache;
};
