#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Input/KeyButtonState.hpp"
#include "MingEngine/Engine/Input/XboxController.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#ifdef GLFW_APIENTRY_DEFINED
#undef APIENTRY
#undef GLFW_APIENTRY_DEFINED
#endif

enum class KeyCode
{
	// 1) Function keys
	F1  = GLFW_KEY_F1,
	F2  = GLFW_KEY_F2,
	F3  = GLFW_KEY_F3,
	F4  = GLFW_KEY_F4,
	F5  = GLFW_KEY_F5,
	F6  = GLFW_KEY_F6,
	F7  = GLFW_KEY_F7,
	F8  = GLFW_KEY_F8,
	F9  = GLFW_KEY_F9,
	F10 = GLFW_KEY_F10,
	F11 = GLFW_KEY_F11,
	F12 = GLFW_KEY_F12,

	// 2) Number row
	Zero  = GLFW_KEY_0,
	One   = GLFW_KEY_1,
	Two   = GLFW_KEY_2,
	Three = GLFW_KEY_3,
	Four  = GLFW_KEY_4,
	Five  = GLFW_KEY_5,
	Six   = GLFW_KEY_6,
	Seven = GLFW_KEY_7,
	Eight = GLFW_KEY_8,
	Nine  = GLFW_KEY_9,

	// 3) Letter keys
	A = GLFW_KEY_A,
	B = GLFW_KEY_B,
	C = GLFW_KEY_C,
	D = GLFW_KEY_D,
	E = GLFW_KEY_E,
	F = GLFW_KEY_F,
	G = GLFW_KEY_G,
	H = GLFW_KEY_H,
	I = GLFW_KEY_I,
	J = GLFW_KEY_J,
	K = GLFW_KEY_K,
	L = GLFW_KEY_L,
	M = GLFW_KEY_M,
	N = GLFW_KEY_N,
	O = GLFW_KEY_O,
	P = GLFW_KEY_P,
	Q = GLFW_KEY_Q,
	R = GLFW_KEY_R,
	S = GLFW_KEY_S,
	T = GLFW_KEY_T,
	U = GLFW_KEY_U,
	V = GLFW_KEY_V,
	W = GLFW_KEY_W,
	X = GLFW_KEY_X,
	Y = GLFW_KEY_Y,
	Z = GLFW_KEY_Z,

	// 4) Special character keys
	Space        = GLFW_KEY_SPACE,
	Apostrophe   = GLFW_KEY_APOSTROPHE,
	Comma        = GLFW_KEY_COMMA,
	Minus        = GLFW_KEY_MINUS,
	Period       = GLFW_KEY_PERIOD,
	Slash        = GLFW_KEY_SLASH,
	Semicolon    = GLFW_KEY_SEMICOLON,
	Equal        = GLFW_KEY_EQUAL,
	LeftBracket  = GLFW_KEY_LEFT_BRACKET,
	Backslash    = GLFW_KEY_BACKSLASH,
	RightBracket = GLFW_KEY_RIGHT_BRACKET,
	Tilde        = GLFW_KEY_GRAVE_ACCENT,

	// 5) Navigation keys
	Esc       = GLFW_KEY_ESCAPE,
	Enter     = GLFW_KEY_ENTER,
	Tab       = GLFW_KEY_TAB,
	Backspace = GLFW_KEY_BACKSPACE,
	Insert    = GLFW_KEY_INSERT,
	Delete    = GLFW_KEY_DELETE,
	Home      = GLFW_KEY_HOME,
	End       = GLFW_KEY_END,
	PageUp    = GLFW_KEY_PAGE_UP,
	PageDown  = GLFW_KEY_PAGE_DOWN,

	// 6) Arrow keys
	UpArrow    = GLFW_KEY_UP,
	DownArrow  = GLFW_KEY_DOWN,
	LeftArrow  = GLFW_KEY_LEFT,
	RightArrow = GLFW_KEY_RIGHT,

	// 7) Lock keys
	CapsLock    = GLFW_KEY_CAPS_LOCK,
	ScrollLock  = GLFW_KEY_SCROLL_LOCK,
	NumLock     = GLFW_KEY_NUM_LOCK,
	PrintScreen = GLFW_KEY_PRINT_SCREEN,
	Pause       = GLFW_KEY_PAUSE,

	// 8) Modifier keys
	Shift        = GLFW_KEY_LEFT_SHIFT,
	LeftControl  = GLFW_KEY_LEFT_CONTROL,
	LeftAlt      = GLFW_KEY_LEFT_ALT,
	LeftSuper    = GLFW_KEY_LEFT_SUPER,
	RightShift   = GLFW_KEY_RIGHT_SHIFT,
	RightControl = GLFW_KEY_RIGHT_CONTROL,
	RightAlt     = GLFW_KEY_RIGHT_ALT,
	RightSuper   = GLFW_KEY_RIGHT_SUPER,
	Menu         = GLFW_KEY_MENU,

	// 9) Numpad keys
	KP0        = GLFW_KEY_KP_0,
	KP1        = GLFW_KEY_KP_1,
	KP2        = GLFW_KEY_KP_2,
	KP3        = GLFW_KEY_KP_3,
	KP4        = GLFW_KEY_KP_4,
	KP5        = GLFW_KEY_KP_5,
	KP6        = GLFW_KEY_KP_6,
	KP7        = GLFW_KEY_KP_7,
	KP8        = GLFW_KEY_KP_8,
	KP9        = GLFW_KEY_KP_9,
	KPDecimal  = GLFW_KEY_KP_DECIMAL,
	KPDivide   = GLFW_KEY_KP_DIVIDE,
	KPMultiply = GLFW_KEY_KP_MULTIPLY,
	KPSubtract = GLFW_KEY_KP_SUBTRACT,
	KPAdd      = GLFW_KEY_KP_ADD,
	KPEnter    = GLFW_KEY_KP_ENTER,
	KPEqual    = GLFW_KEY_KP_EQUAL,

	// 10) Mouse buttons (custom, beyond GLFW)
	LeftMouse  = GLFW_KEY_LAST + 1,
	RightMouse = GLFW_KEY_LAST + 2,
};

constexpr int LastGlfwKeyCode    = GLFW_KEY_LAST;
constexpr int NumKeyCodes        = LastGlfwKeyCode + 3;
constexpr int NumXboxControllers = 4;

constexpr int ToKeyCode(KeyCode input) { return static_cast<int>(input); }

enum class CursorMode
{
	POINTER,
	FPS,
	COUNT
};

struct CursorState
{
	IntVec2 m_cursorClientDelta;
	IntVec2 m_cursorClientPosition;

	CursorMode m_cursorMode = CursorMode::POINTER;
};

struct InputConfig
{
	bool m_isEnable = true;
};

class InputSystem : public SystemBase
{
	MCLASS(InputSystem, SystemBase)

public:
	InputSystem(InputConfig config);
	~InputSystem();

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	bool WasKeyJustPressed(int keyCode);
	bool WasKeyJustReleased(int keyCode);
	bool IsKeyDown(int keyCode);
	bool WasKeyJustPressed(KeyCode input) { return WasKeyJustPressed(ToKeyCode(input)); }
	bool WasKeyJustReleased(KeyCode input) { return WasKeyJustReleased(ToKeyCode(input)); }
	bool IsKeyDown(KeyCode input) { return IsKeyDown(ToKeyCode(input)); }

	void                  HandleKeyPressed(int keyCode);
	void                  HandleKeyReleased(int keyCode);
	void                  HandleKeyPressed(KeyCode input) { HandleKeyPressed(ToKeyCode(input)); }
	void                  HandleKeyReleased(KeyCode input) { HandleKeyReleased(ToKeyCode(input)); }
	XboxController const& GetController(int controllerID);

	void ClearAllInputStates();

	// In pointer mode, the cursor should be visible, freely able to move, and not
	// locked to the window. In FPS mode, the cursor should be hidden, reset to the
	// center of the window each frame, and record the delta each frame.
	void SetCursorMode(CursorMode cursorMode);
	void SetCursorModeByInt(int cursorMode);

	// Returns the current frame cursor delta in pixels, relative to the client
	// region. This is how much the cursor moved last frame before it was reset
	// to the center of the screen. Only valid in FPS mode, will be zero otherwise.
	IntVec2 GetCursorClientDelta() const;

	// Returns the cursor position, in pixels relative to the client region.
	IntVec2 GetCursorClientPosition() const;

	// Returns the cursor position, normalized to the range [0, 1], relative
	// to the client region, with the y-axis inverted to map from Windows
	// conventions to game screen camera conventions
	Vec2 GetCursorNormalizedPosition() const;

	void ClearCursorDelta();

	static bool Event_KeyDown(EventArgs& args);
	static bool Event_KeyUp(EventArgs& args);

protected:
	static void BindMethods();

protected:
	InputConfig m_config;

	CursorMode m_cursorMode = CursorMode::POINTER;

	IntVec2 m_cursorClientPosition     = IntVec2::Zero;
	IntVec2 m_prevCursorClientPosition = IntVec2::Zero;
	IntVec2 m_cursorClientDelta        = IntVec2::Zero;

	KeyButtonState m_keyStates[NumKeyCodes];
	XboxController m_controllers[NumXboxControllers] = {
		XboxController(0), XboxController(1), XboxController(2), XboxController(3)
	};
};
