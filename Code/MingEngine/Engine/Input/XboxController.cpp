#include "MingEngine/Engine/Input/XboxController.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // must #include Windows.h before #including Xinput.h
#include <Xinput.h>  // include the Xinput API header file (interface)

#pragma comment(lib, "xinput") // Link in the xinput.lib static library

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
	return btn.m_isDown;
}

bool XboxController::IsButtonReleased(XboxButtonID buttonID) const
{
	const KeyButtonState& btn = m_buttons[(int)buttonID];
	return !btn.m_isDown;
}

bool XboxController::WasButtonJustPressed(XboxButtonID buttonID) const
{
	const KeyButtonState& btn = m_buttons[(int)buttonID];
	return btn.m_justPressed;
}

bool XboxController::WasButtonJustReleased(XboxButtonID buttonID) const
{
	const KeyButtonState& btn = m_buttons[(int)buttonID];
	return btn.m_justReleased;
}

void XboxController::CollectInputEvents(std::vector<InputEvent>& events) const
{
	XINPUT_STATE sample    = {};
	bool const   connected = XInputGetState(m_id, &sample) == ERROR_SUCCESS;

	// 1) Record connection changes without changing the processed state
	if (connected != m_isConnected)
	{
		InputEvent event;
		event.m_type         = InputEventType::ControllerConnection;
		event.m_controllerID = m_id;
		event.m_connected    = connected;
		events.push_back(event);
	}

	if (!connected)
	{
		return;
	}

	// 2) Queue changed buttons in XboxButtonID order
	unsigned short constexpr buttonFlags[] = { XINPUT_GAMEPAD_A,
											   XINPUT_GAMEPAD_B,
											   XINPUT_GAMEPAD_X,
											   XINPUT_GAMEPAD_Y,
											   XINPUT_GAMEPAD_START,
											   XINPUT_GAMEPAD_BACK,
											   XINPUT_GAMEPAD_DPAD_UP,
											   XINPUT_GAMEPAD_DPAD_DOWN,
											   XINPUT_GAMEPAD_DPAD_LEFT,
											   XINPUT_GAMEPAD_DPAD_RIGHT,
											   XINPUT_GAMEPAD_LEFT_THUMB,
											   XINPUT_GAMEPAD_RIGHT_THUMB,
											   XINPUT_GAMEPAD_LEFT_SHOULDER,
											   XINPUT_GAMEPAD_RIGHT_SHOULDER };
	static_assert(sizeof(buttonFlags) / sizeof(buttonFlags[0]) == static_cast<int>(XboxButtonID::NUM));
	for (int button = 0; button < static_cast<int>(XboxButtonID::NUM); ++button)
	{
		bool const pressed = (sample.Gamepad.wButtons & buttonFlags[button]) != 0;
		if (pressed != m_buttons[button].m_isDown)
		{
			InputEvent event;
			event.m_type             = InputEventType::ControllerButton;
			event.m_controllerID     = m_id;
			event.m_controllerButton = button;
			event.m_pressed          = pressed;
			events.push_back(event);
		}
	}

	// 3) Queue normalized axes and keep deadzone correction in event processing
	auto normalizeStick = [](short value)
	{
		return value < 0 ? value / 32768.f : value / 32767.f;
	};
	float const   axes[]         = { normalizeStick(sample.Gamepad.sThumbLX), normalizeStick(sample.Gamepad.sThumbLY),
									 normalizeStick(sample.Gamepad.sThumbRX), normalizeStick(sample.Gamepad.sThumbRY),
									 sample.Gamepad.bLeftTrigger / 255.f,     sample.Gamepad.bRightTrigger / 255.f };
	Vector2 const left           = m_leftStick.GetRawUncorrectedPosition();
	Vector2 const right          = m_rightStick.GetRawUncorrectedPosition();
	float const   previousAxes[] = { left.x, left.y, right.x, right.y, m_leftTrigger, m_rightTrigger };
	static_assert(sizeof(axes) / sizeof(axes[0]) == static_cast<int>(ControllerAxis::COUNT));
	for (int axis = 0; axis < static_cast<int>(ControllerAxis::COUNT); ++axis)
	{
		if (axes[axis] != previousAxes[axis])
		{
			InputEvent event;
			event.m_type           = InputEventType::ControllerAxis;
			event.m_controllerID   = m_id;
			event.m_controllerAxis = static_cast<ControllerAxis>(axis);
			event.m_axisValue      = axes[axis];
			events.push_back(event);
		}
	}
}

void XboxController::EndFrame()
{
	for (KeyButtonState& button : m_buttons)
	{
		button.m_justPressed  = false;
		button.m_justReleased = false;
		button.m_repeatCount  = 0;
	}
}

void XboxController::Reset()
{
	m_isConnected  = false;
	m_leftTrigger  = 0.f;
	m_rightTrigger = 0.f;
	m_leftStick.Reset();
	m_rightStick.Reset();
	for (int i = 0; i < (int)XboxButtonID::NUM; ++i)
	{
		m_buttons[i] = {};
	}

	SetVibration(0, 0);
}

void XboxController::SetVibration(unsigned short leftMotor, unsigned short rightMotor) const
{
	XINPUT_VIBRATION vib = {};
	vib.wLeftMotorSpeed  = leftMotor;
	vib.wRightMotorSpeed = rightMotor;
	XInputSetState(m_id, &vib);
}
