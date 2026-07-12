#pragma once

#include <cstdint>

#include "ThirdParty/GLFW/glfw3.h"

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
	LeftShift    = GLFW_KEY_LEFT_SHIFT,
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
	RightMouse = GLFW_KEY_LAST + 2
};
inline constexpr int KeyCodeCount = GLFW_KEY_LAST + 3;

// This align with the GLFW key modifier flags, so we can convert directly.
enum class KeyModifier : uint8_t
{
	None    = 0,
	Shift   = 1 << 0,
	Control = 1 << 1,
	Alt     = 1 << 2,
	Super   = 1 << 3
};
constexpr KeyModifier  operator|(KeyModifier lhs, KeyModifier rhs) { return (KeyModifier)((int)lhs | (int)rhs); }
constexpr KeyModifier& operator|=(KeyModifier& lhs, KeyModifier rhs)
{
	lhs = lhs | rhs;
	return lhs;
}

constexpr int ToKeyCode(KeyCode input) { return static_cast<int>(input); }

struct KeyButtonState
{
public:
	bool     m_isDown      = false;
	bool     m_justPressed = false;
	bool     m_justReleased = false;
	uint16_t m_repeatCount = 0;
};

struct KeyboardState
{
	KeyButtonState m_keyStates[KeyCodeCount];
	KeyModifier    m_keyModifiers = KeyModifier::None;
};
