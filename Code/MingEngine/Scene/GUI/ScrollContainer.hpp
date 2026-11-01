#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

class ScrollContainer : public Container
{
	MCLASS(ScrollContainer, Container);

private:
	struct ThemeCache
	{
		Color              background_color;
		int                scrollbar_width = 0;
		int                grabber_minimum_size = 0;
		int                grabber_margin = 0;
		int                scroll_step = 0;
		Ref<ThemeStyleBox> track;
		Ref<ThemeStyleBox> grabber;
		Ref<ThemeStyleBox> grabber_hover;
		Ref<ThemeStyleBox> grabber_pressed;
	};

public:
	ScrollContainer();

	Vector2 GetInternalMinimumSize() const override { return Vector2(32.f, 24.f); }
	float   GetScroll() const { return m_scroll; }
	float   GetHorizontalScroll() const { return m_horizontalScroll; }
	Vector2 GetContentSize() const { return m_contentSize; }
	Vector2 GetPageSize() const { return m_pageSize; }

	void SetScroll(float offset);
	void SetHorizontalScroll(float offset);

	void EnsureControlVisible(Control* control);

	bool GuiInput(InputEvent const& event) override;

protected:
	static void BindMethods();

	void OnThemeChanged() override;

	void OnNotification(int notification);

private:
	Control* GetContent() const;
	float    GetMaximumScroll() const;
	AABB2 GetTrackRect(int axis) const;
	AABB2 GetGrabberRect(int axis) const;

	void Arrange();

private:
	ThemeCache m_themeCache;
	int m_hoverAxis = -1;
	int m_dragScrollbarWidth = 0;
	int m_dragMinimumSize = 0;
	int m_dragMargin = 0;
	float    m_scroll           = 0.f;
	int      m_dragAxis         = -1;
	float    m_dragOffset       = 0.f;
	float    m_horizontalScroll = 0.f;
	Vector2  m_contentSize      = Vector2::Zero;
	Vector2  m_pageSize         = Vector2::Zero;
};
