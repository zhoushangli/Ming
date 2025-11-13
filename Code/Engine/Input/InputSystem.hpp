#pragma once

#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Input/XboxController.hpp"

extern unsigned char const KEYCODE_F1;
extern unsigned char const KEYCODE_F2;
extern unsigned char const KEYCODE_F3;
extern unsigned char const KEYCODE_F4;
extern unsigned char const KEYCODE_F5;
extern unsigned char const KEYCODE_F6;
extern unsigned char const KEYCODE_F7;
extern unsigned char const KEYCODE_F8;
extern unsigned char const KEYCODE_F9;
extern unsigned char const KEYCODE_F10;
extern unsigned char const KEYCODE_F11;
extern unsigned char const KEYCODE_ESC;
extern unsigned char const KEYCODE_UPARROW;
extern unsigned char const KEYCODE_DOWNARROW;
extern unsigned char const KEYCODE_LEFTARROW;
extern unsigned char const KEYCODE_RIGHTARROW;
extern unsigned char const KEYCODE_LEFT_MOUSE;
extern unsigned char const KEYCODE_RIGHT_MOUSE;

constexpr int NUM_KEYCODES = 256;
constexpr int NUM_XBOX_CONTROLLERS = 4;

struct InputConfig
{
	bool m_isEnable = true;
};

class InputSystem
{
public:
	InputSystem(InputConfig config);
	~InputSystem();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	bool WasKeyJustPressed(unsigned char keyCode);
	bool WasKeyJustReleased(unsigned char keyCode);
	bool IsKeyDown(unsigned char keyCode);

	void HandleKeyPressed(unsigned char keyCode);
	void HandleKeyReleased(unsigned char keyCode);
	XboxController const& GetController(int controllerID);

	void ClearAllInputStates();

protected:
	InputConfig		m_config;

	KeyButtonState  m_keyStates[NUM_KEYCODES];
	XboxController  m_controllers[NUM_XBOX_CONTROLLERS] =
		{
			XboxController(0),
			XboxController(1),
			XboxController(2),
			XboxController(3)
		};
};
