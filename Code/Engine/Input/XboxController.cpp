#include "Engine/Input/XboxController.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // must #include Windows.h before #including Xinput.h
#include <Xinput.h> // include the Xinput API header file (interface)

#include "Engine/Math/MathUtils.hpp"
#pragma comment( lib, "xinput" ) // Link in the xinput.lib static library

XboxController::XboxController()
{
}

XboxController::XboxController(int controllerID) : m_id(controllerID)
{
}

bool XboxController::IsConnected() const
{
    return m_isConnected;
}

int XboxController::GetControllerID() const
{
    return m_id;
}

AnalogJoystick const& XboxController::GetLeftStick() const
{
    return m_leftStick;
}

AnalogJoystick const& XboxController::GetRightStick() const
{
    return m_rightStick;
}

float XboxController::GetLeftTrigger() const
{
    return m_leftTrigger;
}

float XboxController::GetRightTrigger() const
{
    return m_rightTrigger;
}

KeyButtonState const& XboxController::GetButton(XboxButtonID buttonID) const
{
    return m_buttons[(int)buttonID];
}

bool XboxController::IsButtonPressed(XboxButtonID buttonID) const
{
    const KeyButtonState& btn = m_buttons[(int)buttonID];
    return btn.m_state;
}

bool XboxController::IsButtonReleased(XboxButtonID buttonID) const
{
    const KeyButtonState& btn = m_buttons[(int)buttonID];
    return !btn.m_state;
}

bool XboxController::WasButtonJustPressed(XboxButtonID buttonID) const
{
    const KeyButtonState& btn = m_buttons[(int)buttonID];
    return btn.m_state && !btn.m_prevState;
}

bool XboxController::WasButtonJustReleased(XboxButtonID buttonID) const
{
    const KeyButtonState& btn = m_buttons[(int)buttonID];
    return !btn.m_state && btn.m_prevState;
}

void XboxController::Update()
{
    XINPUT_STATE xboxControllerState = {};
    DWORD result = XInputGetState(m_id, &xboxControllerState);

    m_isConnected = (result == ERROR_SUCCESS);

    if (!m_isConnected)
    {
        Reset();
        return;
    }

    // Joysticks
    UpdateJoystick(m_leftStick, xboxControllerState.Gamepad.sThumbLX, xboxControllerState.Gamepad.sThumbLY);
    UpdateJoystick(m_rightStick, xboxControllerState.Gamepad.sThumbRX, xboxControllerState.Gamepad.sThumbRY);

    // Triggers
    UpdateTrigger(m_leftTrigger, xboxControllerState.Gamepad.bLeftTrigger);
    UpdateTrigger(m_rightTrigger, xboxControllerState.Gamepad.bRightTrigger);

    // Buttons
    UpdateButton(XboxButtonID::A, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_A);
    UpdateButton(XboxButtonID::B, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_B);
    UpdateButton(XboxButtonID::X, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_X);
    UpdateButton(XboxButtonID::Y, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_Y);
    UpdateButton(XboxButtonID::START, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_START);
    UpdateButton(XboxButtonID::BACK, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_BACK);
    UpdateButton(XboxButtonID::DPAD_UP, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP);
    UpdateButton(XboxButtonID::DPAD_DOWN, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
    UpdateButton(XboxButtonID::DPAD_LEFT, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
    UpdateButton(XboxButtonID::DPAD_RIGHT, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
    UpdateButton(XboxButtonID::LEFT_THUMB, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_THUMB);
    UpdateButton(XboxButtonID::RIGHT_THUMB, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB);
    UpdateButton(XboxButtonID::LEFT_SHOULDER, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
    UpdateButton(XboxButtonID::RIGHT_SHOULDER, xboxControllerState.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
}

void XboxController::Reset()
{
    m_isConnected = false;
    m_leftTrigger = 0.f;
    m_rightTrigger = 0.f;
    m_leftStick.Reset();
    m_rightStick.Reset();
    for (int i = 0; i < (int)XboxButtonID::NUM; ++i)
    {
        m_buttons[i].m_state = false;
        m_buttons[i].m_prevState = false;
    }

    SetVibration(0, 0);
}

void XboxController::UpdateJoystick(AnalogJoystick& out_joystick, short rawX, short rawY)
{
    // Normalize to [-1, 1]
    float normX = (rawX < 0) ? (rawX / 32768.f) : (rawX / 32767.f);
    float normY = (rawY < 0) ? (rawY / 32768.f) : (rawY / 32767.f);
    out_joystick.UpdatePosition(normX, normY);
}

void XboxController::UpdateTrigger(float& out_triggerValue, unsigned char rawValue)
{
    out_triggerValue = rawValue / 255.f;
}

void XboxController::UpdateButton(XboxButtonID buttonID, unsigned short buttonFlags, unsigned short buttonFlag)
{
    KeyButtonState& btn = m_buttons[(int)buttonID];
    btn.m_prevState = btn.m_state;
    btn.m_state = (buttonFlags & buttonFlag) == buttonFlag;
}

void XboxController::SetVibration(unsigned short leftMotor, unsigned short rightMotor) const
{
    XINPUT_VIBRATION vib = {};
    vib.wLeftMotorSpeed = leftMotor;
    vib.wRightMotorSpeed = rightMotor;
    XInputSetState(m_id, &vib);
}
