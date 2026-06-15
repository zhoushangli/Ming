#include "MingEngine/Engine/Input/InputSystem.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"

#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/GLFW/glfw3native.h"

static_assert(LastGlfwKeyCode == GLFW_KEY_LAST);

int const KeyCodeF1         = GLFW_KEY_F1;
int const KeyCodeF2         = GLFW_KEY_F2;
int const KeyCodeF3         = GLFW_KEY_F3;
int const KeyCodeF4         = GLFW_KEY_F4;
int const KeyCodeF5         = GLFW_KEY_F5;
int const KeyCodeF6         = GLFW_KEY_F6;
int const KeyCodeF7         = GLFW_KEY_F7;
int const KeyCodeF8         = GLFW_KEY_F8;
int const KeyCodeF9         = GLFW_KEY_F9;
int const KeyCodeF10        = GLFW_KEY_F10;
int const KeyCodeF11        = GLFW_KEY_F11;
int const KeyCodeF12        = GLFW_KEY_F12;
int const KeyCodeTilde      = GLFW_KEY_GRAVE_ACCENT;
int const KeyCodeEsc        = GLFW_KEY_ESCAPE;
int const KeyCodeUpArrow    = GLFW_KEY_UP;
int const KeyCodeDownArrow  = GLFW_KEY_DOWN;
int const KeyCodeLeftArrow  = GLFW_KEY_LEFT;
int const KeyCodeRightArrow = GLFW_KEY_RIGHT;
int const KeyCodeLeftMouse  = GLFW_KEY_LAST + 1;
int const KeyCodeRightMouse = GLFW_KEY_LAST + 2;

int const KeyCodeShift     = GLFW_KEY_LEFT_SHIFT;
int const KeyCodeEnter     = GLFW_KEY_ENTER;
int const KeyCodeBackspace = GLFW_KEY_BACKSPACE;
int const KeyCodeInsert    = GLFW_KEY_INSERT;
int const KeyCodeDelete    = GLFW_KEY_DELETE;
int const KeyCodeHome      = GLFW_KEY_HOME;
int const KeyCodeEnd       = GLFW_KEY_END;

InputSystem::InputSystem(InputConfig config) : m_config(config) {}

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

	if (g_engine != nullptr && g_engine->m_window != nullptr)
	{
		m_prevCursorClientPosition = m_cursorClientPosition;

		GLFWwindow* window  = g_engine->m_window->GetGLFWWindow();
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

Vec2 InputSystem::GetCursorClientDelta() const { return Vec2(m_cursorClientDelta); }

Vec2 InputSystem::GetCursorClientPosition() const { return Vec2(m_cursorClientPosition); }

Vec2 InputSystem::GetCursorNormalizedPosition() const
{
	if (g_engine == nullptr || g_engine->m_window == nullptr)
	{
		return Vec2::Zero;
	}

	IntVec2 clientDimensions = g_engine->m_window->GetClientDimensions();
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
	g_engine->m_input->HandleKeyPressed(asKey);
	return true;
}

bool InputSystem::Event_KeyUp(EventArgs& args)
{
	int asKey = std::stoi(args.GetValue("asKey", "0"));
	g_engine->m_input->HandleKeyReleased(asKey);
	return true;
}

