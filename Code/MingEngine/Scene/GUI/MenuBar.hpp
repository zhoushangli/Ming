#pragma once

#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"

class PopupPanel;
class PanelContainer;
class VBoxContainer;

class MenuBar : public HBoxContainer
{
	MCLASS(MenuBar, HBoxContainer);

public:
	struct MenuItem
	{
		String                m_text;
		std::function<void()> m_action;
		String                m_shortcut;
		bool                  m_separator = false;
		bool                  m_checkable = false;
		bool                  m_checked   = false;
		bool                  m_enabled   = true;
		String                m_icon;
		std::vector<MenuItem> m_children;
	};

	using MenuProvider = std::function<std::vector<MenuItem>()>;

private:
	struct Menu
	{
		Button*      m_button = nullptr;
		MenuProvider m_provider;
	};

	struct ThemeCache
	{
		int minimum_height      = 24;
		int popup_minimum_width = 200;
	};

public:
	MenuBar();
	~MenuBar() override;

	int     GetActiveMenu() const { return m_active; }
	Vector2 GetInternalMinimumSize() const override;

	void                        AddMenu(String const& title, MenuProvider provider);
	static std::vector<Button*> CreatePopupItems(
		VBoxContainer* content, Control* source, std::vector<MenuItem> const& items, std::function<void()> close,
		std::function<void(Button*, std::vector<MenuItem> const&)> openSubmenu = {});
	static void LayoutPopupItems(std::vector<Button*> const& items);
	void        SetAboutToOpenCallback(std::function<void()> callback) { m_aboutToOpen = std::move(callback); }
	void        CloseMenu(bool restoreFocus = true);

	bool GuiInput(InputEvent const& event) override;

protected:
	static void BindMethods();
	void        OnThemeChanged() override;
	void        OnNotification(int notification);

private:
	void OpenMenu(int index);
	void ResetActiveMenu();
	void LayoutPopup();
	void MoveItemFocus(int direction);
	bool HandlePopupPointer(InputEvent const& event);
	void HandlePopupKey(InputEvent const& event);

private:
	ThemeCache            m_themeCache;
	std::vector<Menu>     m_menus;
	std::vector<Button*>  m_items;
	ObjectID              m_popupID      = ObjectID::Invalid;
	PanelContainer*       m_popupPanel   = nullptr;
	VBoxContainer*        m_popupContent = nullptr;
	int                   m_active       = -1;
	int                   m_selected     = 0;
	std::function<void()> m_aboutToOpen;
};
