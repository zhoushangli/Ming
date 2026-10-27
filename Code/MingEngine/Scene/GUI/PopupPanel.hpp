#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

#include <functional>

class PopupPanel : public Control
{
	MCLASS(PopupPanel, Control);

public:
	PopupPanel();
	void Open(Control* owner);
	void Close(bool restoreFocus = true);
	bool GuiInput(InputEvent const& event) override;
	void SetKeyCallback(std::function<void(InputEvent const&)> callback) { m_keyCallback = std::move(callback); }
	void SetModal(bool modal) { m_modal = modal; }
	Control* GetOwner() const { return ObjectDatabase::GetInstance<Control>(m_owner); }

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	ObjectID                               m_owner = ObjectID::Invalid;
	bool m_modal = false;
	std::function<void(InputEvent const&)> m_keyCallback;
};
