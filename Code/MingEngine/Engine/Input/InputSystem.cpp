#include "MingEngine/Engine/Input/InputSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"

using namespace Math;

InputSystem::InputSystem() : InputSystem(InputSystemConfig{}) {}

InputSystem::InputSystem(InputSystemConfig config) : m_config(config) {}

void InputSystem::BindMethods()
{
	ClassDatabase::BindMethod(
		"IsKeyDown",
		static_cast<bool (InputSystem::*)(int)>(&InputSystem::IsKeyDown),
		{ "keyCode" });
	ClassDatabase::BindMethod(
		"WasKeyJustPressed",
		static_cast<bool (InputSystem::*)(int)>(&InputSystem::WasKeyJustPressed),
		{ "keyCode" });
	ClassDatabase::BindMethod(
		"WasKeyJustReleased",
		static_cast<bool (InputSystem::*)(int)>(&InputSystem::WasKeyJustReleased),
		{ "keyCode" });
	ClassDatabase::BindMethod("SetCursorMode", &InputSystem::SetCursorModeByInt, { "cursorMode" });

	// 1) Function keys
	BIND_CONSTANT(KeyCode, F1);
	BIND_CONSTANT(KeyCode, F2);
	BIND_CONSTANT(KeyCode, F3);
	BIND_CONSTANT(KeyCode, F4);
	BIND_CONSTANT(KeyCode, F5);
	BIND_CONSTANT(KeyCode, F6);
	BIND_CONSTANT(KeyCode, F7);
	BIND_CONSTANT(KeyCode, F8);
	BIND_CONSTANT(KeyCode, F9);
	BIND_CONSTANT(KeyCode, F10);
	BIND_CONSTANT(KeyCode, F11);
	BIND_CONSTANT(KeyCode, F12);

	// 2) Number row
	BIND_CONSTANT(KeyCode, Zero);
	BIND_CONSTANT(KeyCode, One);
	BIND_CONSTANT(KeyCode, Two);
	BIND_CONSTANT(KeyCode, Three);
	BIND_CONSTANT(KeyCode, Four);
	BIND_CONSTANT(KeyCode, Five);
	BIND_CONSTANT(KeyCode, Six);
	BIND_CONSTANT(KeyCode, Seven);
	BIND_CONSTANT(KeyCode, Eight);
	BIND_CONSTANT(KeyCode, Nine);

	// 3) Letter keys
	BIND_CONSTANT(KeyCode, A);
	BIND_CONSTANT(KeyCode, B);
	BIND_CONSTANT(KeyCode, C);
	BIND_CONSTANT(KeyCode, D);
	BIND_CONSTANT(KeyCode, E);
	BIND_CONSTANT(KeyCode, F);
	BIND_CONSTANT(KeyCode, G);
	BIND_CONSTANT(KeyCode, H);
	BIND_CONSTANT(KeyCode, I);
	BIND_CONSTANT(KeyCode, J);
	BIND_CONSTANT(KeyCode, K);
	BIND_CONSTANT(KeyCode, L);
	BIND_CONSTANT(KeyCode, M);
	BIND_CONSTANT(KeyCode, N);
	BIND_CONSTANT(KeyCode, O);
	BIND_CONSTANT(KeyCode, P);
	BIND_CONSTANT(KeyCode, Q);
	BIND_CONSTANT(KeyCode, R);
	BIND_CONSTANT(KeyCode, S);
	BIND_CONSTANT(KeyCode, T);
	BIND_CONSTANT(KeyCode, U);
	BIND_CONSTANT(KeyCode, V);
	BIND_CONSTANT(KeyCode, W);
	BIND_CONSTANT(KeyCode, X);
	BIND_CONSTANT(KeyCode, Y);
	BIND_CONSTANT(KeyCode, Z);

	// 4) Special character keys
	BIND_CONSTANT(KeyCode, Space);
	BIND_CONSTANT(KeyCode, Apostrophe);
	BIND_CONSTANT(KeyCode, Comma);
	BIND_CONSTANT(KeyCode, Minus);
	BIND_CONSTANT(KeyCode, Period);
	BIND_CONSTANT(KeyCode, Slash);
	BIND_CONSTANT(KeyCode, Semicolon);
	BIND_CONSTANT(KeyCode, Equal);
	BIND_CONSTANT(KeyCode, LeftBracket);
	BIND_CONSTANT(KeyCode, Backslash);
	BIND_CONSTANT(KeyCode, RightBracket);
	BIND_CONSTANT(KeyCode, Tilde);

	// 5) Navigation keys
	BIND_CONSTANT(KeyCode, Esc);
	BIND_CONSTANT(KeyCode, Enter);
	BIND_CONSTANT(KeyCode, Tab);
	BIND_CONSTANT(KeyCode, Backspace);
	BIND_CONSTANT(KeyCode, Insert);
	BIND_CONSTANT(KeyCode, Delete);
	BIND_CONSTANT(KeyCode, Home);
	BIND_CONSTANT(KeyCode, End);
	BIND_CONSTANT(KeyCode, PageUp);
	BIND_CONSTANT(KeyCode, PageDown);

	// 6) Arrow keys
	BIND_CONSTANT(KeyCode, UpArrow);
	BIND_CONSTANT(KeyCode, DownArrow);
	BIND_CONSTANT(KeyCode, LeftArrow);
	BIND_CONSTANT(KeyCode, RightArrow);

	// 7) Lock keys
	BIND_CONSTANT(KeyCode, CapsLock);
	BIND_CONSTANT(KeyCode, ScrollLock);
	BIND_CONSTANT(KeyCode, NumLock);
	BIND_CONSTANT(KeyCode, PrintScreen);
	BIND_CONSTANT(KeyCode, Pause);

	// 8) Modifier keys
	BIND_CONSTANT(KeyCode, LeftShift);
	BIND_CONSTANT(KeyCode, LeftControl);
	BIND_CONSTANT(KeyCode, LeftAlt);
	BIND_CONSTANT(KeyCode, LeftSuper);
	BIND_CONSTANT(KeyCode, RightShift);
	BIND_CONSTANT(KeyCode, RightControl);
	BIND_CONSTANT(KeyCode, RightAlt);
	BIND_CONSTANT(KeyCode, RightSuper);
	BIND_CONSTANT(KeyCode, Menu);

	// 9) Numpad keys
	BIND_CONSTANT(KeyCode, KP0);
	BIND_CONSTANT(KeyCode, KP1);
	BIND_CONSTANT(KeyCode, KP2);
	BIND_CONSTANT(KeyCode, KP3);
	BIND_CONSTANT(KeyCode, KP4);
	BIND_CONSTANT(KeyCode, KP5);
	BIND_CONSTANT(KeyCode, KP6);
	BIND_CONSTANT(KeyCode, KP7);
	BIND_CONSTANT(KeyCode, KP8);
	BIND_CONSTANT(KeyCode, KP9);
	BIND_CONSTANT(KeyCode, KPDecimal);
	BIND_CONSTANT(KeyCode, KPDivide);
	BIND_CONSTANT(KeyCode, KPMultiply);
	BIND_CONSTANT(KeyCode, KPSubtract);
	BIND_CONSTANT(KeyCode, KPAdd);
	BIND_CONSTANT(KeyCode, KPEnter);
	BIND_CONSTANT(KeyCode, KPEqual);

	// 10) Mouse buttons
	BIND_CONSTANT(KeyCode, LeftMouse);
	BIND_CONSTANT(KeyCode, RightMouse);

	BIND_ENUM(CursorMode, POINTER);
	BIND_ENUM(CursorMode, FPS);
	BIND_ENUM(CursorMode, COUNT);
}

InputSystem::~InputSystem() {}

void InputSystem::Startup()
{
	m_keyboardState = {};
	m_pendingInputEvents.clear();
	m_cursorClientPosition = IntVec2::Zero;
	ClearCursorDelta();
	m_windowFocused = true;
	SetCursorMode(m_cursorMode);
}

void InputSystem::Shutdown()
{
	m_keyboardState = {};
	m_pendingInputEvents.clear();
}

void InputSystem::BeginFrame()
{
	ClearCursorDelta();

	// 1) Sample controllers into the same queue without changing their processed state
	for (XboxController const& controller : m_controllers)
	{
		controller.CollectInputEvents(m_pendingInputEvents);
	}

	// 2) Take one batch so events added during processing wait for the next frame
	std::vector<InputEvent> const events = GetInputEvents();
	bool constexpr debugInputEvents      = false;
	for (InputEvent const& event : events)
	{
		// 3) Apply each event before observing its resulting state
		ProcessInputEvent(event);
		
		if (debugInputEvents)
		{
			std::string message = "Input ";
			if (event.m_type == InputEventType::Key)
			{
				message += "Key key=" + std::to_string(event.m_keyCode);
				message += " scan=" + std::to_string(event.m_scanCode);
				message += " pressed=" + std::to_string(event.m_pressed);
				message += " repeat=" + std::to_string(event.m_repeat);
				message += " | down=" + std::to_string(IsKeyDown(event.m_keyCode));
				message += " justPressed=" + std::to_string(WasKeyJustPressed(event.m_keyCode));
				message += " justReleased=" + std::to_string(WasKeyJustReleased(event.m_keyCode));
			}
			else if (event.m_type == InputEventType::MouseButton)
			{
				message += "MouseButton button=" + std::to_string(event.m_mouseButton);
				message += " pressed=" + std::to_string(event.m_pressed);
			}
			else if (event.m_type == InputEventType::MouseMotion)
			{
				message += "MouseMotion";
				message += Stringf(" relative=(%.2f,%.2f)", event.m_relative.x, event.m_relative.y);
			}
			else if (event.m_type == InputEventType::MouseWheel)
			{
				message += "MouseWheel";
				message += Stringf(" scroll=(%.2f,%.2f)", event.m_scroll.x, event.m_scroll.y);
			}
			else if (event.m_type == InputEventType::Text)
			{
				message += Stringf("Text codepoint=U+%04X", static_cast<unsigned int>(event.m_codepoint));
			}
			else if (event.m_type == InputEventType::WindowFocus)
			{
				message += "WindowFocus focused=" + std::to_string(event.m_focused);
			}
			else if (event.m_type == InputEventType::CursorEnter)
			{
				message += "CursorEnter entered=" + std::to_string(event.m_entered);
			}
			else if (event.m_type == InputEventType::ControllerConnection)
			{
				message += "ControllerConnection controller=" + std::to_string(event.m_controllerID);
				message += " connected=" + std::to_string(event.m_connected);
			}
			else if (event.m_type == InputEventType::ControllerButton)
			{
				message += "ControllerButton controller=" + std::to_string(event.m_controllerID);
				message += " button=" + std::to_string(event.m_controllerButton);
				message += " pressed=" + std::to_string(event.m_pressed);
			}
			else if (event.m_type == InputEventType::ControllerAxis)
			{
				message += "ControllerAxis controller=" + std::to_string(event.m_controllerID);
				message += " axis=" + std::to_string(static_cast<int>(event.m_controllerAxis));
				message += " value=" + std::to_string(event.m_axisValue);
			}
			else
			{
				message += "None";
			}

			if (event.m_type == InputEventType::MouseButton || event.m_type == InputEventType::MouseMotion
				|| event.m_type == InputEventType::MouseWheel || event.m_type == InputEventType::CursorEnter)
			{
				message += Stringf(" pos=(%.2f,%.2f)", event.m_position.x, event.m_position.y);
				IntVec2 const delta = GetCursorClientDelta();
				message += Stringf(" | frameDelta=(%d,%d)", delta.x, delta.y);
			}
			if (event.m_type == InputEventType::Key || event.m_type == InputEventType::MouseButton
				|| event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseWheel)
			{
				message += " mods=" + std::to_string(event.m_modifiers);
				message += " stateMods=" + std::to_string(static_cast<int>(GetKeyModifiers()));
			}

			INFO_PRINT(message);
		}
	}
}

void InputSystem::EndFrame()
{
	for (int key = 0; key < KeyCodeCount; ++key)
	{
		KeyButtonState& state = m_keyboardState.m_keyStates[key];
		state.m_justPressed   = false;
		state.m_justReleased  = false;
		state.m_repeatCount   = 0;
	}
	for (XboxController& controller : m_controllers)
	{
		controller.EndFrame();
	}
}

bool InputSystem::WasKeyJustPressed(int keyCode)
{
	if (keyCode < 0 || keyCode >= KeyCodeCount)
	{
		return false;
	}

	return m_keyboardState.m_keyStates[keyCode].m_justPressed;
}

bool InputSystem::WasKeyJustReleased(int keyCode)
{
	if (keyCode < 0 || keyCode >= KeyCodeCount)
	{
		return false;
	}

	return m_keyboardState.m_keyStates[keyCode].m_justReleased;
}

bool InputSystem::IsKeyDown(int keyCode)
{
	if (keyCode < 0 || keyCode >= KeyCodeCount)
	{
		return false;
	}

	return m_keyboardState.m_keyStates[keyCode].m_isDown;
}

void InputSystem::ProcessInputEvent(InputEvent const& event)
{
	if (event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseButton
		|| event.m_type == InputEventType::MouseWheel)
	{
		m_cursorClientPosition = IntVec2(static_cast<int>(event.m_position.x), static_cast<int>(event.m_position.y));
	}

	int keyCode = -1;
	switch (event.m_type)
	{
	case InputEventType::Key:
		keyCode = event.m_keyCode;
		break;
	case InputEventType::MouseButton:
		if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT)
		{
			keyCode = ToKeyCode(KeyCode::LeftMouse);
		}
		else if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
		{
			keyCode = ToKeyCode(KeyCode::RightMouse);
		}
		break;
	case InputEventType::MouseMotion:
		if (m_windowFocused && m_cursorMode == CursorMode::FPS)
		{
			m_cursorClientDelta += event.m_relative;
		}
		break;
	case InputEventType::Text:
		if (event.m_codepoint <= 255)
		{
			EventArgs args;
			args.SetValue("asKey", std::to_string(event.m_codepoint));
			FireEvent("CharInput", args);
		}
		break;
	case InputEventType::WindowFocus:
		m_windowFocused = event.m_focused;
		if (!m_windowFocused)
		{
			ClearAllInputStates();
		}
		break;
	case InputEventType::ControllerConnection:
	case InputEventType::ControllerButton:
	case InputEventType::ControllerAxis:
	{
		if (event.m_controllerID < 0 || event.m_controllerID >= ControllerCount)
		{
			return;
		}
		XboxController& controller = m_controllers[event.m_controllerID];
		if (event.m_type == InputEventType::ControllerConnection)
		{
			if (!event.m_connected)
			{
				controller.Reset();
			}
			else if (m_windowFocused)
			{
				controller.m_isConnected = true;
			}
			return;
		}
		if (!m_windowFocused || !controller.m_isConnected)
		{
			return;
		}

		if (event.m_type == InputEventType::ControllerButton)
		{
			if (event.m_controllerButton < 0 || event.m_controllerButton >= static_cast<int>(XboxButtonID::NUM))
			{
				return;
			}
			KeyButtonState& button = controller.m_buttons[event.m_controllerButton];
			button.m_justPressed  |= event.m_pressed && !button.m_isDown;
			button.m_justReleased |= !event.m_pressed && button.m_isDown;
			button.m_isDown = event.m_pressed;
		}
		else
		{
			Vector2 const left  = controller.m_leftStick.GetRawUncorrectedPosition();
			Vector2 const right = controller.m_rightStick.GetRawUncorrectedPosition();
			switch (event.m_controllerAxis)
			{
			case ControllerAxis::LeftStickX:
				controller.m_leftStick.UpdatePosition(event.m_axisValue, left.y);
				break;
			case ControllerAxis::LeftStickY:
				controller.m_leftStick.UpdatePosition(left.x, event.m_axisValue);
				break;
			case ControllerAxis::RightStickX:
				controller.m_rightStick.UpdatePosition(event.m_axisValue, right.y);
				break;
			case ControllerAxis::RightStickY:
				controller.m_rightStick.UpdatePosition(right.x, event.m_axisValue);
				break;
			case ControllerAxis::LeftTrigger:
				controller.m_leftTrigger = event.m_axisValue;
				break;
			case ControllerAxis::RightTrigger:
				controller.m_rightTrigger = event.m_axisValue;
				break;
			default:
				break;
			}
		}
		break;
	}
	default:
		break;
	}

	// 1) Apply the modifiers carried by this event, including extra mouse buttons
	if (event.m_type == InputEventType::Key || event.m_type == InputEventType::MouseButton
		|| event.m_type == InputEventType::MouseMotion || event.m_type == InputEventType::MouseWheel)
	{
		int constexpr modifierMask     = GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER;
		m_keyboardState.m_keyModifiers = static_cast<KeyModifier>(event.m_modifiers & modifierMask);
	}

	// 2) Update the key state without clearing earlier transitions in this frame
	if (keyCode >= 0 && keyCode < KeyCodeCount)
	{
		KeyButtonState& state = m_keyboardState.m_keyStates[keyCode];
		state.m_isDown        = event.m_pressed;
		if (event.m_repeat)
		{
			++state.m_repeatCount;
		}
		else if (event.m_pressed)
		{
			state.m_justPressed = true;
		}
		else
		{
			state.m_justReleased = true;
		}
	}
}

XboxController const& InputSystem::GetController(int controllerID)
{
	if (controllerID < 0 || controllerID >= ControllerCount)
	{
		return m_controllers[0];
	}

	return m_controllers[controllerID];
}

void InputSystem::ClearAllInputStates()
{
	m_keyboardState = {};
	ClearCursorDelta();

	for (int i = 0; i < ControllerCount; ++i)
	{
		m_controllers[i].Reset();
	}
}

void InputSystem::SetCursorMode(CursorMode cursorMode)
{
	if (cursorMode != CursorMode::POINTER && cursorMode != CursorMode::FPS)
	{
		return;
	}
	if (m_cursorMode != cursorMode)
	{
		m_cursorMode = cursorMode;
		ClearCursorDelta();
	}
	if (g_engine != nullptr && g_engine->m_windowSystem != nullptr)
	{
		g_engine->m_windowSystem->SetCursorMode(cursorMode);
	}
}

void InputSystem::SetCursorModeByInt(int cursorMode)
{
	if (cursorMode < 0 || cursorMode >= static_cast<int>(CursorMode::COUNT))
	{
		return;
	}

	SetCursorMode(static_cast<CursorMode>(cursorMode));
}

IntVec2 InputSystem::GetCursorClientDelta() const
{
	return IntVec2(static_cast<int>(m_cursorClientDelta.x), static_cast<int>(m_cursorClientDelta.y));
}

IntVec2 InputSystem::GetCursorClientPosition() const { return m_cursorClientPosition; }

Vector2 InputSystem::GetCursorNormalizedPosition() const
{
	if (g_engine == nullptr || g_engine->m_windowSystem == nullptr)
	{
		return Vector2::Zero;
	}

	IntVec2 clientDimensions = g_engine->m_windowSystem->GetClientDimensions();
	if (clientDimensions.x <= 0 || clientDimensions.y <= 0)
	{
		return Vector2::Zero;
	}

	IntVec2 clientPos = GetCursorClientPosition();

	float u = (float)clientPos.x / (float)clientDimensions.x;
	float v = (float)clientPos.y / (float)clientDimensions.y;

	u = GetClamped(u, 0.f, 1.f);
	v = GetClamped(v, 0.f, 1.f);

	v = 1.f - v;
	return Vector2(u, v);
}

void InputSystem::ClearCursorDelta() { m_cursorClientDelta = Vector2::Zero; }

void InputSystem::AddInputEvent(InputEvent const& event) { m_pendingInputEvents.push_back(event); }

std::vector<InputEvent> InputSystem::GetInputEvents()
{
	std::vector<InputEvent> events;
	events.swap(m_pendingInputEvents);
	return events;
}
