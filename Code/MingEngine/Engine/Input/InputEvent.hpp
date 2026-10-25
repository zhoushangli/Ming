#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

#include <cstdint>

enum class InputEventType
{
	None,
	MouseMotion,
	MouseButton,
	MouseWheel,
	Key,
	Text,
	WindowFocus,
	CursorEnter,
	ControllerConnection,
	ControllerButton,
	ControllerAxis
};

enum class ControllerAxis
{
	LeftStickX,
	LeftStickY,
	RightStickX,
	RightStickY,
	LeftTrigger,
	RightTrigger,
	COUNT
};

struct InputEvent
{
	InputEventType m_type = InputEventType::None;

	Vector2 m_position = Vector2::Zero;
	Vector2 m_relative = Vector2::Zero;
	Vector2 m_scroll   = Vector2::Zero;

	int m_keyCode     = -1;
	int m_scanCode    = -1;
	int m_mouseButton = -1;
	int m_modifiers   = 0;

	uint32_t m_codepoint = 0;

	int            m_controllerID     = -1;
	int            m_controllerButton = -1;
	ControllerAxis m_controllerAxis   = ControllerAxis::COUNT;
	float          m_axisValue        = 0.f;
	bool           m_connected        = false;

	bool m_pressed = false;
	bool m_repeat  = false;
	bool m_focused = false;
	bool m_entered = false;
};
