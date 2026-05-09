#pragma once

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Input/XboxController.hpp"

extern unsigned char const kKeyCodeF1;
extern unsigned char const kKeyCodeF2;
extern unsigned char const kKeyCodeF3;
extern unsigned char const kKeyCodeF4;
extern unsigned char const kKeyCodeF5;
extern unsigned char const kKeyCodeF6;
extern unsigned char const kKeyCodeF7;
extern unsigned char const kKeyCodeF8;
extern unsigned char const kKeyCodeF9;
extern unsigned char const kKeyCodeF10;
extern unsigned char const kKeyCodeF11;
extern unsigned char const kKeyCodeF12;
extern unsigned char const kKeyCodeTilde;
extern unsigned char const kKeyCodeEsc;
extern unsigned char const kKeyCodeUpArrow;
extern unsigned char const kKeyCodeDownArrow;
extern unsigned char const kKeyCodeLeftArrow;
extern unsigned char const kKeyCodeRightArrow;
extern unsigned char const kKeyCodeLeftMouse;
extern unsigned char const kKeyCodeRightMouse;

extern unsigned char const kKeyCodeShift;
extern unsigned char const kKeyCodeEnter;
extern unsigned char const kKeyCodeBackspace;
extern unsigned char const kKeyCodeInsert;
extern unsigned char const kKeyCodeDelete;
extern unsigned char const kKeyCodeHome;
extern unsigned char const kKeyCodeEnd;

constexpr int kNumKeyCodes = 256;
constexpr int kNumXboxControllers = 4;

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

    // In pointer mode, the cursor should be visible, freely able to move, and not
	// locked to the window. In FPS mode, the cursor should be hidden, reset to the
	// center of the window each frame, and record the delta each frame.
    void SetCursorMode(CursorMode cursorMode);

    // Returns the current frame cursor delta in pixels, relative to the client
    // region. This is how much the cursor moved last frame before it was reset
    // to the center of the screen. Only valid in FPS mode, will be zero otherwise.
    Vec2 GetCursorClientDelta() const;

    // Returns the cursor position, in pixels relative to the client region.
    Vec2 GetCursorClientPosition() const;

    // Returns the cursor position, normalized to the range [0, 1], relative
    // to the client region, with the y-axis inverted to map from Windows
    // conventions to game screen camera conventions
    Vec2 GetCursorNormalizedPosition() const;

	void ClearCursorDelta();

	static bool Event_KeyDown(EventArgs& args);
    static bool Event_KeyUp(EventArgs& args);

protected:
	InputConfig		m_config;

    CursorMode		m_cursorMode = CursorMode::POINTER;

    IntVec2         m_cursorClientPosition = IntVec2::kZero;
    IntVec2         m_prevCursorClientPosition = IntVec2::kZero;
    IntVec2         m_cursorClientDelta = IntVec2::kZero;

	KeyButtonState  m_keyStates[kNumKeyCodes];
	XboxController  m_controllers[kNumXboxControllers] =
		{
			XboxController(0),
			XboxController(1),
			XboxController(2),
			XboxController(3)
		};
};


