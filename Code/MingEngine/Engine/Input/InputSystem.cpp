#include "MingEngine/Engine/Input/InputSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"

using namespace Math;

#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/GLFW/glfw3native.h"

InputSystem::InputSystem(InputSystemConfig config) : m_config(config) {}

void InputSystem::BindMethods()
{
	ClassDatabase::BindMethod("IsKeyDown", static_cast<bool (InputSystem::*)(int)>(&InputSystem::IsKeyDown));
	ClassDatabase::BindMethod(
		"WasKeyJustPressed",
		static_cast<bool (InputSystem::*)(int)>(&InputSystem::WasKeyJustPressed));
	ClassDatabase::BindMethod(
		"WasKeyJustReleased",
		static_cast<bool (InputSystem::*)(int)>(&InputSystem::WasKeyJustReleased));
	ClassDatabase::BindMethod("SetCursorMode", &InputSystem::SetCursorModeByInt);

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

void InputSystem::Startup() { m_keyboardState = {}; }

void InputSystem::Shutdown() { m_keyboardState = {}; }

void InputSystem::BeginFrame()
{
	for (int i = 0; i < ControllerCount; ++i)
	{
		m_controllers[i].Update();
	}

	if (g_engine != nullptr && g_engine->m_windowSystem != nullptr)
	{
		m_prevCursorClientPosition = m_cursorClientPosition;

		GLFWwindow* window  = g_engine->m_windowSystem->GetGLFWWindow();
		double      cursorX = 0.0;
		double      cursorY = 0.0;
		glfwGetCursorPos(window, &cursorX, &cursorY);
		m_cursorClientPosition = IntVec2((int)cursorX, (int)cursorY);

		if (m_cursorMode == CursorMode::FPS)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

			m_cursorClientDelta = m_cursorClientPosition - m_prevCursorClientPosition;
		}
		else if (m_cursorMode == CursorMode::POINTER)
		{
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

			m_cursorClientDelta = IntVec2::Zero;
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

void InputSystem::HandleKeyCallback(int keyCode, int action, int mods)
{
	if (keyCode < 0 || keyCode >= KeyCodeCount)
	{
		return;
	}

	KeyButtonState& state = m_keyboardState.m_keyStates[keyCode];

	switch (action)
	{
	case GLFW_REPEAT: // This will trigger when you hold down a key
		state.m_isDown = true;
		state.m_repeatCount++;
		break;
	case GLFW_PRESS: // This will trigger when you just press a key
		state.m_isDown      = true;
		state.m_justPressed = true;
		break;
	case GLFW_RELEASE: // This will trigger when you just release a key
		state.m_isDown       = false;
		state.m_justReleased = true;
		break;
	}

	KeyModifier result = KeyModifier::None;
	if (mods & GLFW_MOD_SHIFT)
		result |= KeyModifier::Shift;
	if (mods & GLFW_MOD_CONTROL)
		result |= KeyModifier::Control;
	if (mods & GLFW_MOD_ALT)
		result |= KeyModifier::Alt;
	if (mods & GLFW_MOD_SUPER)
		result |= KeyModifier::Super;

	m_keyboardState.m_keyModifiers = result;
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

	for (int i = 0; i < ControllerCount; ++i)
	{
		m_controllers[i].Reset();
	}
}

void InputSystem::SetCursorMode(CursorMode cursorMode) { m_cursorMode = cursorMode; }

void InputSystem::SetCursorModeByInt(int cursorMode)
{
	if (cursorMode < 0 || cursorMode >= static_cast<int>(CursorMode::COUNT))
	{
		return;
	}

	SetCursorMode(static_cast<CursorMode>(cursorMode));
}

IntVec2 InputSystem::GetCursorClientDelta() const { return m_cursorClientDelta; }

IntVec2 InputSystem::GetCursorClientPosition() const { return m_cursorClientPosition; }

Vec2 InputSystem::GetCursorNormalizedPosition() const
{
	if (g_engine == nullptr || g_engine->m_windowSystem == nullptr)
	{
		return Vec2::Zero;
	}

	IntVec2 clientDimensions = g_engine->m_windowSystem->GetClientDimensions();
	if (clientDimensions.x <= 0 || clientDimensions.y <= 0)
	{
		return Vec2::Zero;
	}

	IntVec2 clientPos = GetCursorClientPosition();

	float u = (float)clientPos.x / (float)clientDimensions.x;
	float v = (float)clientPos.y / (float)clientDimensions.y;

	u = GetClamped(u, 0.f, 1.f);
	v = GetClamped(v, 0.f, 1.f);

	v = 1.f - v;
	return Vec2(u, v);
}

void InputSystem::ClearCursorDelta() { m_cursorClientDelta = IntVec2::Zero; }
