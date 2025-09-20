#pragma once

//#include "Engine/Math/Vec2.hpp"
//#include "KeyButtonState.hpp"
//
//class AnalogJoystick
//{
//public:
//	Vec2  GetPosition() const;
//	float GetMagnitude() const;
//	float GetOrientationDegrees() const;
//
//	Vec2  GetRawUncorrectedPosition() const;
//	float GetInnerDeadZoneFraction() const;
//	float GetOuterDeadZoneFraction() const;
//
//	// For use by XboxController, et al.
//	void  Reset();
//	void  SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold);
//	void  UpdatePosition(float rawNormalizedX, float rawNormalizedY);
//
//protected:
//	Vec2  m_rawPosition;             // Flaky; doesn't rest at zero (or consistently snap to rest position)
//	Vec2  m_correctedPosition;       // Deadzone-corrected position
//	float m_innerDeadZoneFraction = 0.0f;  // If R < this%, R = 0; "input range start" for corrective range map
//	float m_outerDeadZoneFraction = 1.00f; // if R > this%, R = 1; "input range end" for corrective range map
//};
//
//
//class XboxController
//{
//	friend class InputSystem;
//
//public:
//	XboxController();
//	~XboxController();
//	bool  IsConnected() const;
//	int   GetControllerID() const;
//	AnalogJoystick const& GetLeftStick() const;
//	AnalogJoystick const& GetRightStick() const;
//	float GetLeftTrigger() const;
//	float GetRightTrigger() const;
//	KeyButtonState const& GetButton(XboxButtonID buttonID) const;
//	bool  IsButtonDown(XboxButtonID buttonID) const;
//	bool  WasButtonJustPressed(XboxButtonID buttonID) const;
//	bool  WasButtonJustReleased(XboxButtonID buttonID) const;
//
//private:
//	void Update();
//	void Reset();
//	void UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY);
//	void UpdateTrigger(float& out_triggerValue, unsigned char rawValue);
//	void UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonFlag);
//
//private:
//	int            m_id = -1;
//	bool           m_isConnected = false;
//	float          m_leftTrigger = 0.f;
//	float          m_rightTrigger = 0.f;
//	KeyButtonState m_buttons[(int)XboxButtonID::NUM];
//	AnalogJoystick m_leftStick;
//	AnalogJoystick m_rightStick;
//};


