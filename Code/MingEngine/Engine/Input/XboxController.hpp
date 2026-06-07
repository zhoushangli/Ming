#pragma once

#include "MingEngine/Engine/Input/AnalogJoystick.hpp"
#include "MingEngine/Engine/Input/KeyButtonState.hpp"

enum class XboxButtonID
{
	A,
	B,
	X,
	Y,
	START,
	BACK,
	DPAD_UP,
	DPAD_DOWN,
	DPAD_LEFT,
	DPAD_RIGHT,
	LEFT_THUMB,
	RIGHT_THUMB,
	LEFT_SHOULDER,
	RIGHT_SHOULDER,
	NUM
};

class XboxController
{
	friend class InputSystem;

public:
	XboxController();
	XboxController(int controllerID);
	bool					IsConnected() const;
	int						GetControllerID() const;

	AnalogJoystick const&	GetLeftStick() const;
	AnalogJoystick const&	GetRightStick() const;
	float					GetLeftTrigger() const;
	float					GetRightTrigger() const;

	KeyButtonState const&	GetButton(XboxButtonID buttonID) const;
	bool					IsButtonPressed(XboxButtonID buttonID) const;
	bool					IsButtonReleased(XboxButtonID buttonID) const;
	bool					WasButtonJustPressed(XboxButtonID buttonID) const;
	bool					WasButtonJustReleased(XboxButtonID buttonID) const;

	void					SetVibration(unsigned short leftMotor = 0.f, unsigned short rightMotor = 0.f) const;

private:
	void Update();
	void Reset();
	void UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY);
	void UpdateTrigger(float& out_triggerValue, unsigned char rawValue);
	void UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonFlag);
	

private:
	int m_id = -1;
	bool m_isConnected = false;
	float m_leftTrigger = 0.f;
	float m_rightTrigger = 0.f;
	KeyButtonState m_buttons[(int)XboxButtonID::NUM];
	AnalogJoystick m_leftStick;
	AnalogJoystick m_rightStick;
};
