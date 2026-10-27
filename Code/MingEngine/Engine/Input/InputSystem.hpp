#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Math/IntVec2.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Engine/Input/InputEvent.hpp"
#include "MingEngine/Engine/Input/KeyboardState.hpp"
#include "MingEngine/Engine/Input/XboxController.hpp"

#include "ThirdParty/GLFW/glfw3.h"
#ifdef GLFW_APIENTRY_DEFINED
#undef APIENTRY
#undef GLFW_APIENTRY_DEFINED
#endif

#include <functional>
#include <vector>

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

	bool IsKeyDown(int keyCode);

	bool IsKeyDown(KeyCode input) { return IsKeyDown(ToKeyCode(input)); }

	bool WasKeyJustPressed(int keyCode);
	bool WasKeyJustReleased(int keyCode);

	bool        WasKeyJustPressed(KeyCode input) { return WasKeyJustPressed(ToKeyCode(input)); }
	bool        WasKeyJustReleased(KeyCode input) { return WasKeyJustReleased(ToKeyCode(input)); }
	KeyModifier GetKeyModifiers() const { return m_keyboardState.m_keyModifiers; }

	XboxController const& GetController(int controllerID);

	void ClearAllInputStates();

	// Immediately switch between a visible pointer and GLFW's disabled cursor mode.
	// e.g. SetCursorMode(CursorMode::FPS) captures the cursor for camera movement.
	void SetCursorMode(CursorMode cursorMode);
	void SetCursorModeByInt(int cursorMode);

	// Return accumulated motion from processed events in FPS mode, or zero in pointer mode.
	// e.g. Two motions of (2, 1) and (3, -1) produce a frame delta of (5, 0).
	IntVec2 GetCursorClientDelta() const;

	// Returns the cursor position, in pixels relative to the client region.
	IntVec2 GetCursorClientPosition() const;

	// Returns the cursor position, normalized to the range [0, 1], relative
	// to the client region, with the y-axis inverted to map from Windows
	// conventions to game screen camera conventions
	Vector2 GetCursorNormalizedPosition() const;

	void ClearCursorDelta();

	void                    AddInputEvent(InputEvent const& event);
	std::vector<InputEvent> GetInputEvents();
	void                    ProcessInputEvent(InputEvent const& event);
	void                    SetInputEventCallback(std::function<void(InputEvent const&)> callback);

protected:
	static void BindMethods();

protected:
	// +3 for LeftMouse, RightMouse, Count
	static int constexpr ControllerCount = 4;

	InputSystemConfig m_config;

	CursorMode m_cursorMode = CursorMode::POINTER;

	IntVec2 m_cursorClientPosition = IntVec2::Zero;
	Vector2 m_cursorClientDelta    = Vector2::Zero;

	KeyboardState  m_keyboardState;
	XboxController m_controllers[ControllerCount] = {
		XboxController(0), XboxController(1), XboxController(2), XboxController(3)
	};

private:
	std::vector<InputEvent>                m_pendingInputEvents;
	std::function<void(InputEvent const&)> m_inputEventCallback;
	bool                                   m_windowFocused = true;
};
