#pragma once

#include "Engine/Core/EventSystem.hpp"
#include "Engine/Input/KeyButtonState.hpp"
#include "Engine/Input/XboxController.hpp"

extern unsigned char const KeyCodeF1;
extern unsigned char const KeyCodeF2;
extern unsigned char const KeyCodeF3;
extern unsigned char const KeyCodeF4;
extern unsigned char const KeyCodeF5;
extern unsigned char const KeyCodeF6;
extern unsigned char const KeyCodeF7;
extern unsigned char const KeyCodeF8;
extern unsigned char const KeyCodeF9;
extern unsigned char const KeyCodeF10;
extern unsigned char const KeyCodeF11;
extern unsigned char const KeyCodeF12;
extern unsigned char const KeyCodeTilde;
extern unsigned char const KeyCodeEsc;
extern unsigned char const KeyCodeUpArrow;
extern unsigned char const KeyCodeDownArrow;
extern unsigned char const KeyCodeLeftArrow;
extern unsigned char const KeyCodeRightArrow;
extern unsigned char const KeyCodeLeftMouse;
extern unsigned char const KeyCodeRightMouse;

extern unsigned char const KeyCodeShift;
extern unsigned char const KeyCodeEnter;
extern unsigned char const KeyCodeBackspace;
extern unsigned char const KeyCodeInsert;
extern unsigned char const KeyCodeDelete;
extern unsigned char const KeyCodeHome;
extern unsigned char const KeyCodeEnd;

constexpr int kNumKeyCodes        = 256;
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

	void                  HandleKeyPressed(unsigned char keyCode);
	void                  HandleKeyReleased(unsigned char keyCode);
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
	InputConfig m_config;

	CursorMode m_cursorMode = CursorMode::POINTER;

	IntVec2 m_cursorClientPosition     = IntVec2::Zero;
	IntVec2 m_prevCursorClientPosition = IntVec2::Zero;
	IntVec2 m_cursorClientDelta        = IntVec2::Zero;

	KeyButtonState m_keyStates[kNumKeyCodes];
	XboxController m_controllers[kNumXboxControllers] = {
		XboxController(0), XboxController(1), XboxController(2), XboxController(3)};
};
