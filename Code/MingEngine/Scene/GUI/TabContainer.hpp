#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"

class TabContainer : public Container
{
	MCLASS(TabContainer, Container);

public:
	TabContainer();
	void     AddPage(String const& title, Control* page);
	void     SetCurrentTab(int index);
	int      GetCurrentTab() const { return m_current; }
	Control* GetCurrentPage() const;
	Vector2  GetInternalMinimumSize() const override { return Vector2(160.f, 80.f); }
	void     SetTabChangedCallback(std::function<void(int)> callback) { m_changed = std::move(callback); }

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	std::vector<Control*>    m_pages;
	std::vector<Button*>     m_buttons;
	int                      m_current = -1;
	std::function<void(int)> m_changed;
};
