#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

#include <functional>

// SplitContainer is a container that splits its area into two panels
// Our editor screen is basically mutiple SplitContainer stacked each other
class SplitContainer : public Container
{
	MCLASS(SplitContainer, Container);

private:
	struct ThemeCache
	{
		int                separation = 0;
		int                grabber_length = 0;
		int                grabber_width = 0;
		int                grabber_separation = 0;
		Color              grabber_color;
		Color              grabber_hover_color;
		Color              grabber_pressed_color;
		Ref<ThemeStyleBox> normal;
		Ref<ThemeStyleBox> hover;
		Ref<ThemeStyleBox> pressed;
	};

public:
	SplitContainer();

	bool    IsVertical() const { return m_vertical; }
	float   GetSplitRatio() const { return m_ratio; }
	float   GetSplitOffset() const { return m_offset; }
	Vector2 GetInternalMinimumSize() const override;
	AABB2 GetDividerRect() const;

	void SetVertical(bool vertical);
	void SetSplitRatio(float ratio);
	void SetDragFinishedCallback(std::function<void()> callback) { m_finished = std::move(callback); }

	bool GuiInput(InputEvent const& event) override;
	CursorShape GetCursorShape(Vector2 const& localPosition) const override;

protected:
	static void BindMethods();

	void OnThemeChanged() override;
	void        OnNotification(int notification);

private:
	void GetPanels(Control*& first, Control*& second) const;
	float GetSeparation() const;

private:
	ThemeCache m_themeCache;
	bool m_hovering = false;
	int m_appliedSeparation = 0;
	bool                  m_vertical   = false;
	bool                  m_dragging   = false;
	float                 m_ratio      = 0.5f;
	float                 m_offset     = 0.f;
	float                 m_grabOffset = 0.f;
	std::function<void()> m_finished;
};
