#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"

class TabContainer : public Container
{
	MCLASS(TabContainer, Container);

public:
	TabContainer();

	int      GetCurrentTab() const { return m_current; }
	Control* GetCurrentPage() const;
	int GetPageCount() const { return static_cast<int>(m_pages.size()); }
	Control* GetPage(int index) const;
	Button* GetTabButton(int index) const;
	int GetPageIndex(Control const* page) const;
	int GetTabInsertIndex(Vector2 const& position) const;
	String GetPageTitle(int index) const;
	Vector2  GetInternalMinimumSize() const override { return Vector2(160.f, 80.f); }

	void SetCurrentTab(int index);

	void SetTabChangedCallback(std::function<void(int)> callback) { m_changed = std::move(callback); }
	void SetTabDragCallback(std::function<bool(Control*, GuiDragData&)> callback) { m_drag = std::move(callback); }
	void SetTabsRearrangeGroup(int group) { m_rearrangeGroup = group; }
	void SetDropInsertIndex(int index);

	void AddPage(String const& title, Control* page);
	void InsertPage(String const& title, Control* page, int index);
	Control* RemovePage(int index);
	void MovePage(int from, int to);

	bool CanDropData(Vector2 const& position, GuiDragData const& data) const override;
	void DropData(Vector2 const& position, GuiDragData const& data) override;
	void DragHover(Vector2 const& position, GuiDragData const& data, bool accepted) override;

protected:
	static void BindMethods() {}

	void OnNotification(int notification);

private:
	void HoverTab(Control* page, Vector2 const& position, GuiDragData const& data);

private:
	std::vector<Control*>    m_pages;
	std::vector<Button*>     m_buttons;
	int                      m_current = -1;
	std::function<void(int)> m_changed;
	std::function<bool(Control*, GuiDragData&)> m_drag;
	int m_rearrangeGroup = -1;
	int m_dropIndex = -1;
	ObjectID m_hoverPage = ObjectID::Invalid;
	double m_hoverStarted = 0.0;
};
