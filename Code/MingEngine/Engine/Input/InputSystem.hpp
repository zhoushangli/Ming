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

enum class Input
{
	F1         = GLFW_KEY_F1,
	F2         = GLFW_KEY_F2,
	F3         = GLFW_KEY_F3,
	F4         = GLFW_KEY_F4,
	F5         = GLFW_KEY_F5,
	F6         = GLFW_KEY_F6,
	F7         = GLFW_KEY_F7,
	F8         = GLFW_KEY_F8,
	F9         = GLFW_KEY_F9,
	F10        = GLFW_KEY_F10,
	F11        = GLFW_KEY_F11,
	F12        = GLFW_KEY_F12,
	Tilde      = GLFW_KEY_GRAVE_ACCENT,
	Esc        = GLFW_KEY_ESCAPE,
	UpArrow    = GLFW_KEY_UP,
	DownArrow  = GLFW_KEY_DOWN,
	LeftArrow  = GLFW_KEY_LEFT,
	RightArrow = GLFW_KEY_RIGHT,
	LeftMouse  = GLFW_KEY_LAST + 1,
	RightMouse = GLFW_KEY_LAST + 2,

	Shift     = GLFW_KEY_LEFT_SHIFT,
	Enter     = GLFW_KEY_ENTER,
	Backspace = GLFW_KEY_BACKSPACE,
	Insert    = GLFW_KEY_INSERT,
	Delete    = GLFW_KEY_DELETE,
	Home      = GLFW_KEY_HOME,
	End       = GLFW_KEY_END,
};

constexpr int LastGlfwKeyCode    = GLFW_KEY_LAST;
constexpr int NumKeyCodes        = LastGlfwKeyCode + 3;
constexpr int NumXboxControllers = 4;

constexpr int ToKeyCode(Input input) { return static_cast<int>(input); }

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
	bool WasKeyJustPressed(Input input) { return WasKeyJustPressed(ToKeyCode(input)); }
	bool WasKeyJustReleased(Input input) { return WasKeyJustReleased(ToKeyCode(input)); }
	bool IsKeyDown(Input input) { return IsKeyDown(ToKeyCode(input)); }

	void                  HandleKeyPressed(int keyCode);
	void                  HandleKeyReleased(int keyCode);
	void                  HandleKeyPressed(Input input) { HandleKeyPressed(ToKeyCode(input)); }
	void                  HandleKeyReleased(Input input) { HandleKeyReleased(ToKeyCode(input)); }
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
