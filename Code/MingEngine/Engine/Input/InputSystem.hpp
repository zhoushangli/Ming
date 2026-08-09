#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Engine/Input/KeyboardState.hpp"
#include "MingEngine/Engine/Input/XboxController.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#ifdef GLFW_APIENTRY_DEFINED
#undef APIENTRY
#undef GLFW_APIENTRY_DEFINED
#endif

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

struct InputSystemConfig
{
	bool m_isEnable = true;
};

class InputSystem : public SystemBase
{
	MCLASS(InputSystem, SystemBase)

public:
	InputSystem();
	InputSystem(InputSystemConfig config);
	~InputSystem();

	void Startup() override;
	void Shutdown() override;
	void BeginFrame() override;
	void EndFrame() override;

	bool        IsKeyDown(int keyCode);
	bool        IsKeyDown(KeyCode input) { return IsKeyDown(ToKeyCode(input)); }
	bool        WasKeyJustPressed(int keyCode);
	bool        WasKeyJustReleased(int keyCode);
	bool        WasKeyJustPressed(KeyCode input) { return WasKeyJustPressed(ToKeyCode(input)); }
	bool        WasKeyJustReleased(KeyCode input) { return WasKeyJustReleased(ToKeyCode(input)); }
	KeyModifier GetKeyModifiers() const { return m_keyboardState.m_keyModifiers; }

	void                  HandleKeyCallback(int keyCode, int action, int mods);
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
	Vector2 GetCursorNormalizedPosition() const;

	void ClearCursorDelta();

protected:
	static void BindMethods();

protected:
	// +3 for LeftMouse, RightMouse, Count
	static int constexpr ControllerCount = 4;

	InputSystemConfig m_config;

	CursorMode m_cursorMode = CursorMode::POINTER;

	IntVec2 m_cursorClientPosition     = IntVec2::Zero;
	IntVec2 m_prevCursorClientPosition = IntVec2::Zero;
	IntVec2 m_cursorClientDelta        = IntVec2::Zero;

	KeyboardState  m_keyboardState;
	XboxController m_controllers[ControllerCount] = {
		XboxController(0), XboxController(1), XboxController(2), XboxController(3)
	};
};
