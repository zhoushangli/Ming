#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Input/KeyButtonState.hpp"
#include "MingEngine/Engine/Input/XboxController.hpp"

extern int const KeyCodeF1;
extern int const KeyCodeF2;
extern int const KeyCodeF3;
extern int const KeyCodeF4;
extern int const KeyCodeF5;
extern int const KeyCodeF6;
extern int const KeyCodeF7;
extern int const KeyCodeF8;
extern int const KeyCodeF9;
extern int const KeyCodeF10;
extern int const KeyCodeF11;
extern int const KeyCodeF12;
extern int const KeyCodeTilde;
extern int const KeyCodeEsc;
extern int const KeyCodeUpArrow;
extern int const KeyCodeDownArrow;
extern int const KeyCodeLeftArrow;
extern int const KeyCodeRightArrow;
extern int const KeyCodeLeftMouse;
extern int const KeyCodeRightMouse;

extern int const KeyCodeShift;
extern int const KeyCodeEnter;
extern int const KeyCodeBackspace;
extern int const KeyCodeInsert;
extern int const KeyCodeDelete;
extern int const KeyCodeHome;
extern int const KeyCodeEnd;

constexpr int LastGlfwKeyCode    = 348;
constexpr int NumKeyCodes        = LastGlfwKeyCode + 3;
constexpr int NumXboxControllers = 4;

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

	void                  HandleKeyPressed(int keyCode);
	void                  HandleKeyReleased(int keyCode);
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
