#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

#include <functional>

// SplitContainer is a container that splits its area into two panels
// Our editor screen is basically mutiple SplitContainer stacked each other
class SplitContainer : public Container
{
	MCLASS(SplitContainer, Container);

public:
	SplitContainer();

	bool    IsVertical() const { return m_vertical; }
	float   GetSplitRatio() const { return m_ratio; }
	float   GetSplitOffset() const { return m_offset; }
	Vector2 GetInternalMinimumSize() const override;

	void SetVertical(bool vertical);
	void SetSplitRatio(float ratio);
	void SetDragFinishedCallback(std::function<void()> callback) { m_finished = std::move(callback); }

	bool GuiInput(InputEvent const& event) override;

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	void GetPanels(Control*& first, Control*& second) const;

private:
	bool                  m_vertical   = false;
	bool                  m_dragging   = false;
	float                 m_ratio      = 0.5f;
	float                 m_offset     = 0.f;
	float                 m_grabOffset = 0.f;
	std::function<void()> m_finished;
};
