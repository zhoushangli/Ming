#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

#include <functional>

class BaseButton : public Control
{
	MCLASS(BaseButton, Control);

public:
	enum class DrawMode
	{
		Normal,
		Hover,
		Pressed,
		HoverPressed,
		Disabled
	};

	BaseButton();
	bool GuiInput(InputEvent const& event) override;

	bool IsDisabled() const override { return m_status.m_disabled; }
	bool IsToggleMode() const { return m_toggleMode; }
	bool IsPressed() const { return m_toggleMode ? m_status.m_pressed : m_status.m_pressAttempt; }
	bool IsPressing() const { return m_status.m_pressAttempt; }
	bool IsHovered() const { return m_status.m_hovering; }

	DrawMode GetDrawMode() const;
	void     SetDisabled(bool disabled);
	void     SetToggleMode(bool toggleMode);
	void     SetPressed(bool pressed);
	void     SetPressedNoSignal(bool pressed);

	void SetButtonDownCallback(std::function<void()> callback) { m_buttonDownCallback = std::move(callback); }
	void SetButtonUpCallback(std::function<void()> callback) { m_buttonUpCallback = std::move(callback); }
	void SetPressedCallback(std::function<void()> callback) { m_pressedCallback = std::move(callback); }
	void SetToggledCallback(std::function<void(bool)> callback) { m_toggledCallback = std::move(callback); }

protected:
	static void BindMethods() {}

	void OnNotification(int notification);

private:
	void CompletePress(bool activate);

	struct Status
	{
		bool m_hovering       = false;
		bool m_pressAttempt   = false;
		bool m_pressingInside = false;
		bool m_pressed        = false;
		bool m_disabled       = false;
		bool m_mousePress     = false;
		int  m_activationKey  = -1;
	};

	Status                    m_status;
	bool                      m_toggleMode = false;
	std::function<void()>     m_buttonDownCallback;
	std::function<void()>     m_buttonUpCallback;
	std::function<void()>     m_pressedCallback;
	std::function<void(bool)> m_toggledCallback;
};
