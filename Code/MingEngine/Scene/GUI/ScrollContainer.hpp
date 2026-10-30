#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

class ScrollContainer : public Container
{
	MCLASS(ScrollContainer, Container);

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
	static void BindMethods() {}

	void OnNotification(int notification);

private:
	Control* GetContent() const;
	float    GetMaximumScroll() const;

	void Arrange();

private:
	float    m_scroll           = 0.f;
	int      m_dragAxis         = -1;
	float    m_dragOffset       = 0.f;
	float    m_horizontalScroll = 0.f;
	Vector2  m_contentSize      = Vector2::Zero;
	Vector2  m_pageSize         = Vector2::Zero;
};
