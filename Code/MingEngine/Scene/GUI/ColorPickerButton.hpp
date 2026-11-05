#pragma once

#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/ColorPicker.hpp"

class PopupPanel;
class ScrollContainer;

class ColorPickerButton : public Button
{
	MCLASS(ColorPickerButton, Button);

public:
	ColorPickerButton();
	~ColorPickerButton() override;

	Color        GetColor() const { return m_color; }
	ColorPicker* GetPicker();
	bool         IsPickerOpen() const { return m_editing; }
	bool         IsPickingScreen() const { return m_picking; }
	Vector2      GetInternalMinimumSize() const override;

	void SetColorNoSignal(Color const& color);
	void SetColorChangedCallback(std::function<void(Color const&)> callback) { m_changed = std::move(callback); }
	void SetEditingStartedCallback(std::function<void()> callback) { m_started = std::move(callback); }
	void SetEditingEndedCallback(std::function<void(bool)> callback) { m_ended = std::move(callback); }
	void SetScreenColorReader(ColorPicker::ScreenColorReader reader);
	void OpenPicker();
	void ClosePicker(bool commit = true, bool restoreFocus = true);
	void StartScreenPicking();

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	void PlacePopup();
	void PopupClosed();
	void HandleKey(InputEvent const& event);
	bool HandlePointer(InputEvent const& event);
	void FinishScreenPicking(bool accept);
	void Preview(Color const& color);

private:
	Color                             m_color       = Color::White;
	Color                             m_original    = Color::White;
	Color                             m_beforePick  = Color::White;
	ObjectID                          m_popupID     = ObjectID::Invalid;
	ColorPicker*                      m_picker      = nullptr;
	ScrollContainer*                  m_scroll      = nullptr;
	bool                              m_editing     = false;
	bool                              m_picking     = false;
	bool                              m_accept      = true;
	bool                              m_validSample = false;
	std::function<void(Color const&)> m_changed;
	std::function<void()>             m_started;
	std::function<void(bool)>         m_ended;
	ColorPicker::ScreenColorReader    m_screenReader;
};
