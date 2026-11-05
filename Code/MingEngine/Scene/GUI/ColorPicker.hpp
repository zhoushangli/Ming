#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

#include <array>
#include <functional>

class Button;
class Label;
class LineEdit;
class SpinBox;

class ColorPicker : public Control
{
	MCLASS(ColorPicker, Control);

public:
	using ScreenColorReader = std::function<bool(Vector2 const&, Color&)>;

private:
	struct ThemeCache
	{
		int                minimum_width = 320;
		int                shape_height  = 200;
		int                bar_width     = 20;
		int                row_height    = 28;
		int                spacing       = 8;
		int                padding       = 8;
		int                checker_size  = 6;
		Color              checker_light;
		Color              checker_dark;
		Color              cursor_color;
		Ref<ThemeStyleBox> panel;
	};

public:
	ColorPicker();

	Color   GetColor() const { return m_color; }
	bool    IsHSVMode() const { return m_hsvMode; }
	Vector2 GetInternalMinimumSize() const override;
	AABB2   GetSelectionRect() const;
	AABB2   GetHueRect() const;
	AABB2   GetAlphaRect() const;
	AABB2   GetChannelRect(int index) const;
	AABB2   GetOldColorRect() const;

	void SetColorNoSignal(Color const& color);
	void SetOldColor(Color const& color);
	void SetHSVMode(bool enabled);
	void SetColorChangedCallback(std::function<void(Color const&)> callback) { m_changed = std::move(callback); }
	void SetPickRequestedCallback(std::function<void()> callback) { m_pickRequested = std::move(callback); }
	void SetScreenColorReader(ScreenColorReader reader);
	bool ReadScreenColor(Vector2 const& position, Color& color) const;
	void EndEditing(bool commit);
	bool HandleKey(InputEvent const& event);
	bool GuiInput(InputEvent const& event) override;

protected:
	static void BindMethods();
	void        OnThemeChanged() override;
	void        OnNotification(int notification);

private:
	void  LayoutChildren();
	void  SyncFields();
	void  UpdateHSV();
	void  Publish(Color const& color, bool keepHSV = false);
	void  SetChannel(int index, float value);
	void  ApplyPointer(Vector2 const& position);
	void  DrawContent();
	void  DrawChecker(AABB2 const& rect);
	void  DrawMarker(AABB2 const& rect, float value, bool vertical);
	void  UpdateTextures();
	float GetChannel(int index) const;
	float FirstRow() const;
	float RowPitch() const;

private:
	Color                               m_color      = Color::White;
	Color                               m_oldColor   = Color::White;
	float                               m_hue        = 0.f;
	float                               m_saturation = 0.f;
	float                               m_value      = 1.f;
	bool                                m_hsvMode    = false;
	int                                 m_dragRegion = -1;
	std::array<Label*, 4>               m_labels     = {};
	std::array<SpinBox*, 4>             m_spins      = {};
	Button*                             m_rgbButton  = nullptr;
	Button*                             m_hsvButton  = nullptr;
	Button*                             m_pickButton = nullptr;
	Label*                              m_hexLabel   = nullptr;
	LineEdit*                           m_hex        = nullptr;
	std::array<Ref<TextureResource>, 6> m_gradients;
	std::array<std::array<float, 4>, 6> m_gradientKeys = {};
	std::function<void(Color const&)>   m_changed;
	std::function<void()>               m_pickRequested;
	ScreenColorReader                   m_screenReader;
	ThemeCache                          m_themeCache;
};
