#include "Engine/Input/InputSystem.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <Windows.h>

unsigned char const KEYCODE_F1			= VK_F1;
unsigned char const KEYCODE_F2			= VK_F2;
unsigned char const KEYCODE_F3			= VK_F3;
unsigned char const KEYCODE_F4			= VK_F4;
unsigned char const KEYCODE_F5			= VK_F5;
unsigned char const KEYCODE_F6			= VK_F6;
unsigned char const KEYCODE_F7			= VK_F7;
unsigned char const KEYCODE_F8			= VK_F8;
unsigned char const KEYCODE_F9			= VK_F9;
unsigned char const KEYCODE_F10			= VK_F10;
unsigned char const KEYCODE_F11			= VK_F11;
unsigned char const KEYCODE_F12			= VK_F12;
unsigned char const KEYCODE_TILDE       = VK_OEM_3;
unsigned char const KEYCODE_ESC			= VK_ESCAPE;
unsigned char const KEYCODE_UPARROW		= VK_UP;
unsigned char const KEYCODE_DOWNARROW	= VK_DOWN;
unsigned char const KEYCODE_LEFTARROW	= VK_LEFT;
unsigned char const KEYCODE_RIGHTARROW	= VK_RIGHT;
unsigned char const KEYCODE_LEFT_MOUSE  = VK_LBUTTON;
unsigned char const KEYCODE_RIGHT_MOUSE = VK_RBUTTON;

unsigned char const KEYCODE_ENTER     = VK_RETURN;
unsigned char const KEYCODE_BACKSPACE = VK_BACK;
unsigned char const KEYCODE_INSERT    = VK_INSERT;
unsigned char const KEYCODE_DELETE    = VK_DELETE;
unsigned char const KEYCODE_HOME      = VK_HOME;
unsigned char const KEYCODE_END       = VK_END;


InputSystem::InputSystem(InputConfig config) : m_config(config)
{

}

InputSystem::~InputSystem()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].m_state = false;
		m_keyStates[key].m_prevState = false;
	}
}

void InputSystem::Startup()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].m_state = false;
		m_keyStates[key].m_prevState = false;
	}

    g_engine->m_eventSystem->SubscribeEventCallbackFunction("KeyUp", InputSystem::Event_KeyUp);
    g_engine->m_eventSystem->SubscribeEventCallbackFunction("KeyDown", InputSystem::Event_KeyDown);
}

void InputSystem::Shutdown()
{
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("KeyDown", Event_KeyDown);
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("KeyUp", Event_KeyUp);

	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].m_state = false;
		m_keyStates[key].m_prevState = false;
	}
}

void InputSystem::BeginFrame()
{
	for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
	{
		m_controllers[i].Update();
	}

    if (g_engine != nullptr && g_engine->m_window != nullptr)
    {
        HWND hwnd = (HWND)g_engine->m_window->GetHwnd();
        if (hwnd != nullptr)
        {
            // 1) Cursor visibility (Windows uses an internal show/hide counter)
            bool const shouldHideCursor = (m_cursorMode == CursorMode::FPS);
            if (shouldHideCursor)
            {
                while (::ShowCursor(FALSE) >= 0) {}
            }
            else
            {
                while (::ShowCursor(TRUE) < 0) {}
            }

            // 2) Cache last frame cursor position
            m_prevCursorClientPosition = m_cursorClientPosition;

            // 3) Read current cursor position (client pixels)
            auto GetCursorClientPosInt = [hwnd]() -> IntVec2
            {
                POINT cursorScreen{};
                ::GetCursorPos(&cursorScreen);

                POINT cursorClient = cursorScreen;
                ::ScreenToClient(hwnd, &cursorClient);

                return IntVec2(cursorClient.x, cursorClient.y);
            };

            m_cursorClientPosition = GetCursorClientPosInt();

            // 4) Relative mode: compute delta then recenter to client middle
            if (m_cursorMode == CursorMode::FPS)
            {
                m_cursorClientDelta = m_cursorClientPosition - m_prevCursorClientPosition;

                RECT clientRect{};
                ::GetClientRect(hwnd, &clientRect);

                int const clientWidth = clientRect.right - clientRect.left;
                int const clientHeight = clientRect.bottom - clientRect.top;

                POINT clientCenter{ clientWidth / 2, clientHeight / 2 };

                POINT centerScreen = clientCenter;
                ::ClientToScreen(hwnd, &centerScreen);
                ::SetCursorPos(centerScreen.x, centerScreen.y);

                // Re-read (Windows may delay the SetCursorPos)
                m_cursorClientPosition = GetCursorClientPosInt();
            }
            else
            {
                // 5) Pointer mode: no relative delta
                m_cursorClientDelta = IntVec2::ZERO;
            }
        }
    }
}

void InputSystem::EndFrame()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].m_prevState = m_keyStates[key].m_state;
	}
}

bool InputSystem::WasKeyJustPressed(unsigned char keyCode)
{
	return m_keyStates[keyCode].m_state && !m_keyStates[keyCode].m_prevState;
}

bool InputSystem::WasKeyJustReleased(unsigned char keyCode)
{
	return !m_keyStates[keyCode].m_state && m_keyStates[keyCode].m_prevState;
}

bool InputSystem::IsKeyDown(unsigned char keyCode)
{
	return m_keyStates[keyCode].m_state;
}

void InputSystem::HandleKeyPressed(unsigned char keyCode)
{
	m_keyStates[keyCode].m_state = true;
}

void InputSystem::HandleKeyReleased(unsigned char keyCode)
{
	m_keyStates[keyCode].m_state = false;
}

XboxController const& InputSystem::GetController(int controllerID)
{
	if (controllerID < 0 || controllerID >= NUM_XBOX_CONTROLLERS) 
	{
		return m_controllers[0];
	}

	return m_controllers[controllerID];
}

void InputSystem::ClearAllInputStates()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].m_state = false;
		m_keyStates[key].m_prevState = false;
	}

	for (int i = 0; i < NUM_XBOX_CONTROLLERS; ++i)
	{
		m_controllers[i].Reset();
	}
}

void InputSystem::SetCursorMode(CursorMode cursorMode)
{
    m_cursorMode = cursorMode;
}

Vec2 InputSystem::GetCursorClientDelta() const
{
    return Vec2(m_cursorClientDelta);
}

Vec2 InputSystem::GetCursorClientPosition() const
{
    return Vec2(m_cursorClientPosition);
}

Vec2 InputSystem::GetCursorNormalizedPosition() const
{
    if (g_engine == nullptr || g_engine->m_window == nullptr)
    {
        return Vec2(0.f, 0.f);
    }

    HWND hwnd = (HWND)g_engine->m_window->GetHwnd();
    if (hwnd == nullptr)
    {
        return Vec2(0.f, 0.f);
    }

    RECT clientRect{};
    ::GetClientRect(hwnd, &clientRect);

    float const clientWidth = (float)(clientRect.right - clientRect.left);
    float const clientHeight = (float)(clientRect.bottom - clientRect.top);
    if (clientWidth <= 0.f || clientHeight <= 0.f)
    {
        return Vec2(0.f, 0.f);
    }

    Vec2 clientPos = GetCursorClientPosition();

    float u = clientPos.x / clientWidth;
    float v = clientPos.y / clientHeight;

    u = GetClamped(u, 0.f, 1.f);
    v = GetClamped(v, 0.f, 1.f);

    v = 1.f - v;
    return Vec2(u, v);
}

void InputSystem::ClearCursorDelta()
{
    m_cursorClientDelta = IntVec2::ZERO;
}

bool InputSystem::Event_KeyDown(EventArgs& args)
{
    unsigned char asKey = (unsigned char)std::stoi(args.GetValue("asKey", "0"));
    g_engine->m_input->HandleKeyPressed(asKey);
    return true;
}

bool InputSystem::Event_KeyUp(EventArgs& args)
{
    unsigned char asKey = (unsigned char)std::stoi(args.GetValue("asKey", "0"));
    g_engine->m_input->HandleKeyReleased(asKey);
    return true;
}