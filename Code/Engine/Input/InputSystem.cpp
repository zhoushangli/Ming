#include "InputSystem.hpp"

#include <Windows.h>

unsigned char const KEYCODE_F7			= VK_F7;
unsigned char const KEYCODE_F8			= VK_F8;
unsigned char const KEYCODE_F9			= VK_F9;
unsigned char const KEYCODE_F10			= VK_F10;
unsigned char const KEYCODE_F11			= VK_F11;
unsigned char const KEYCODE_ESC			= VK_ESCAPE;
unsigned char const KEYCODE_UPARROW		= VK_UP;
unsigned char const KEYCODE_DOWNARROW	= VK_DOWN;
unsigned char const KEYCODE_LEFTARROW	= VK_LEFT;
unsigned char const KEYCODE_RIGHTARROW	= VK_RIGHT;

InputSystem::InputSystem()
{

}

InputSystem::~InputSystem()
{

}

void InputSystem::Startup()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].state = false;
		m_keyStates[key].prevState = false;
	}
}

void InputSystem::Shutdown()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].state = false;
		m_keyStates[key].prevState = false;
	}
}

void InputSystem::BeginFrame()
{
	for (int key = 0; key < 256; ++key)
	{
		bool isDown = (GetAsyncKeyState(key) & 0x8000) != 0;
		if (isDown)
		{
			HandleKeyPressed((unsigned char)key);
		}
		else
		{
			HandleKeyReleased((unsigned char)key);
		}
	}
}

void InputSystem::EndFrame()
{
	for (int key = 0; key < 256; ++key)
	{
		m_keyStates[key].prevState = m_keyStates[key].state;
	}
}

bool InputSystem::WasKeyJustPressed(unsigned char keyCode)
{
	return m_keyStates[keyCode].state && !m_keyStates[keyCode].prevState;
}

bool InputSystem::WasKeyJustReleased(unsigned char keyCode)
{
	return !m_keyStates[keyCode].state && m_keyStates[keyCode].prevState;
}

bool InputSystem::IsKeyDown(unsigned char keyCode)
{
	return m_keyStates[keyCode].state;
}

void InputSystem::HandleKeyPressed(unsigned char keyCode)
{
	m_keyStates[keyCode].state = true;
}

void InputSystem::HandleKeyReleased(unsigned char keyCode)
{
	m_keyStates[keyCode].state = false;
}
