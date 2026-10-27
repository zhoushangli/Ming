#pragma once

#include "MingEngine/Scene/GUI/Button.hpp"

class PopupPanel;
class ScrollContainer;

class OptionButton : public Button
{
	MCLASS(OptionButton, Button);

public:
	struct Item
	{
		String m_name;
		int    m_value = 0;
	};

	OptionButton();
	~OptionButton() override;
	bool SetOptions(std::string const& hint);
	void SetValueNoSignal(int value);
	int  GetValue() const { return m_value; }
	void SetValueChangedCallback(std::function<void(int)> callback) { m_changed = std::move(callback); }
	void OpenOptions();
	void CloseOptions(bool restoreFocus = true);
	std::vector<Item> const& GetOptions() const { return m_items; }

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	void                     Select(int index);
	void                     UpdateActive();
	void                     HandleKey(InputEvent const& event);
	std::vector<Item>        m_items;
	std::vector<Button*>     m_buttons;
	ObjectID                 m_popupID = ObjectID::Invalid;
	ScrollContainer*         m_scroll  = nullptr;
	int                      m_value   = 0;
	int                      m_active  = 0;
	std::function<void(int)> m_changed;
};
