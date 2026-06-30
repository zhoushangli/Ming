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

static_assert(LastGlfwKeyCode == GLFW_KEY_LAST);

InputSystem::InputSystem(InputConfig config) : m_config(config) {}

void InputSystem::BindMethods()
{
	ClassDatabase::BindMethod("IsKeyDown", static_cast<bool (InputSystem::*)(int)>(&InputSystem::IsKeyDown));
	ClassDatabase::BindMethod(
		"WasKeyJustPressed", static_cast<bool (InputSystem::*)(int)>(&InputSystem::WasKeyJustPressed));
	ClassDatabase::BindMethod(
		"WasKeyJustReleased", static_cast<bool (InputSystem::*)(int)>(&InputSystem::WasKeyJustReleased));
	ClassDatabase::BindMethod("SetCursorMode", &InputSystem::SetCursorModeByInt);
	ClassDatabase::BindMethod("GetCursorClientDelta", &InputSystem::GetCursorClientDelta);

	BIND_ENUM(Input, F1);
	BIND_ENUM(Input, F2);
	BIND_ENUM(Input, F3);
	BIND_ENUM(Input, F4);
	BIND_ENUM(Input, F5);
	BIND_ENUM(Input, F6);
	BIND_ENUM(Input, F7);
	BIND_ENUM(Input, F8);
	BIND_ENUM(Input, F9);
	BIND_ENUM(Input, F10);
	BIND_ENUM(Input, F11);
	BIND_ENUM(Input, F12);
	BIND_ENUM(Input, Tilde);
	BIND_ENUM(Input, Esc);
	BIND_ENUM(Input, UpArrow);
	BIND_ENUM(Input, DownArrow);
	BIND_ENUM(Input, LeftArrow);
	BIND_ENUM(Input, RightArrow);
	BIND_ENUM(Input, LeftMouse);
	BIND_ENUM(Input, RightMouse);
	BIND_ENUM(Input, Shift);
	BIND_ENUM(Input, Enter);
	BIND_ENUM(Input, Backspace);
	BIND_ENUM(Input, Insert);
	BIND_ENUM(Input, Delete);
	BIND_ENUM(Input, Home);
	BIND_ENUM(Input, End);

	BIND_ENUM(CursorMode, POINTER);
	BIND_ENUM(CursorMode, FPS);
	BIND_ENUM(CursorMode, COUNT);
}

InputSystem::~InputSystem()
{
	for (int key = 0; key < NumKeyCodes; ++key)
	{
		m_keyStates[key].m_state     = false;
		m_keyStates[key].m_prevState = false;
	}
}

void InputSystem::Startup()
{
	for (int key = 0; key < NumKeyCodes; ++key)
	{
		m_keyStates[key].m_state     = false;
		m_keyStates[key].m_prevState = false;
	}

	g_engine->m_eventSystem->RegisterEvent("KeyUp", InputSystem::Event_KeyUp);
	g_engine->m_eventSystem->RegisterEvent("KeyDown", InputSystem::Event_KeyDown);
}

void InputSystem::Shutdown()
{
	g_engine->m_eventSystem->UnregisterEvent("KeyDown", Event_KeyDown);
	g_engine->m_eventSystem->UnregisterEvent("KeyUp", Event_KeyUp);

	for (int key = 0; key < NumKeyCodes; ++key)
	{
		m_keyStates[key].m_state     = false;
		m_keyStates[key].m_prevState = false;
	}
}

void InputSystem::BeginFrame()
{
	for (int i = 0; i < NumXboxControllers; ++i)
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
	for (int key = 0; key < NumKeyCodes; ++key)
	{
		m_keyStates[key].m_prevState = m_keyStates[key].m_state;
	}
}

bool InputSystem::WasKeyJustPressed(int keyCode)
{
	if (keyCode < 0 || keyCode >= NumKeyCodes)
	{
		return false;
	}

	return m_keyStates[keyCode].m_state && !m_keyStates[keyCode].m_prevState;
}

bool InputSystem::WasKeyJustReleased(int keyCode)
{
	if (keyCode < 0 || keyCode >= NumKeyCodes)
	{
		return false;
	}

	return !m_keyStates[keyCode].m_state && m_keyStates[keyCode].m_prevState;
}

bool InputSystem::IsKeyDown(int keyCode)
{
	if (keyCode < 0 || keyCode >= NumKeyCodes)
	{
		return false;
	}

	return m_keyStates[keyCode].m_state;
}

void InputSystem::HandleKeyPressed(int keyCode)
{
	if (keyCode < 0 || keyCode >= NumKeyCodes)
	{
		return;
	}

	m_keyStates[keyCode].m_state = true;
}

void InputSystem::HandleKeyReleased(int keyCode)
{
	if (keyCode < 0 || keyCode >= NumKeyCodes)
	{
		return;
	}

	m_keyStates[keyCode].m_state = false;
}

XboxController const& InputSystem::GetController(int controllerID)
{
	if (controllerID < 0 || controllerID >= NumXboxControllers)
	{
		return m_controllers[0];
	}

	return m_controllers[controllerID];
}

void InputSystem::ClearAllInputStates()
{
	for (int key = 0; key < NumKeyCodes; ++key)
	{
		m_keyStates[key].m_state     = false;
		m_keyStates[key].m_prevState = false;
	}

	for (int i = 0; i < NumXboxControllers; ++i)
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

Vec2 InputSystem::GetCursorClientDelta() const { return Vec2(m_cursorClientDelta); }

Vec2 InputSystem::GetCursorClientPosition() const { return Vec2(m_cursorClientPosition); }

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

	Vec2 clientPos = GetCursorClientPosition();

	float u = clientPos.x / (float)clientDimensions.x;
	float v = clientPos.y / (float)clientDimensions.y;

	u = GetClamped(u, 0.f, 1.f);
	v = GetClamped(v, 0.f, 1.f);

	v = 1.f - v;
	return Vec2(u, v);
}

void InputSystem::ClearCursorDelta() { m_cursorClientDelta = IntVec2::Zero; }

bool InputSystem::Event_KeyDown(EventArgs& args)
{
	int asKey = std::stoi(args.GetValue("asKey", "0"));
	g_engine->m_inputSystem->HandleKeyPressed(asKey);
	return true;
}

bool InputSystem::Event_KeyUp(EventArgs& args)
{
	int asKey = std::stoi(args.GetValue("asKey", "0"));
	g_engine->m_inputSystem->HandleKeyReleased(asKey);
	return true;
}
