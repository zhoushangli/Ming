#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/GLFW/glfw3native.h"
// #include "ThirdParty/imgui/backends/imgui_impl_glfw.h"

#pragma comment(lib, "ThirdParty/GLFW/glfw3.lib")

namespace
{
Vector2 s_previousMousePosition = Vector2::Zero;
bool    s_hasPreviousMousePosition = false;
int     s_previousCursorMode = GLFW_CURSOR_NORMAL;

static void AddInputEvent(InputEvent const& event)
{
	if (g_engine != nullptr && g_engine->m_inputSystem != nullptr)
	{
		g_engine->m_inputSystem->AddInputEvent(event);
	}
}

static Vector2 GetMousePosition(GLFWwindow* window)
{
	double x = 0.0;
	double y = 0.0;
	glfwGetCursorPos(window, &x, &y);
	return Vector2(static_cast<float>(x), static_cast<float>(y));
}

static int GetMouseModifiers(GLFWwindow* window)
{
	int mods = 0;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
		|| glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
	{
		mods |= GLFW_MOD_SHIFT;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
		|| glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS)
	{
		mods |= GLFW_MOD_CONTROL;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS
		|| glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS)
	{
		mods |= GLFW_MOD_ALT;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS
		|| glfwGetKey(window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS)
	{
		mods |= GLFW_MOD_SUPER;
	}
	return mods;
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

	(void)window;

	InputEvent event;
	event.m_type      = InputEventType::Key;
	event.m_keyCode   = key;
	event.m_scanCode  = scancode;
	event.m_modifiers = mods;
	event.m_pressed   = action != GLFW_RELEASE;
	event.m_repeat    = action == GLFW_REPEAT;
	AddInputEvent(event);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	// ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

	InputEvent event;
	event.m_type        = InputEventType::MouseButton;
	event.m_position    = GetMousePosition(window);
	event.m_mouseButton = button;
	event.m_modifiers   = mods;
	event.m_pressed     = action == GLFW_PRESS;
	AddInputEvent(event);
}

static void CursorPosCallback(GLFWwindow* window, double x, double y)
{
	Vector2 const position(static_cast<float>(x), static_cast<float>(y));
	int const cursorMode = glfwGetInputMode(window, GLFW_CURSOR);

	InputEvent event;
	event.m_type      = InputEventType::MouseMotion;
	event.m_position  = position;
	event.m_modifiers = GetMouseModifiers(window);
	if (s_hasPreviousMousePosition && cursorMode == s_previousCursorMode)
	{
		event.m_relative = position - s_previousMousePosition;
	}

	s_previousMousePosition    = position;
	s_hasPreviousMousePosition = true;
	s_previousCursorMode       = cursorMode;
	AddInputEvent(event);
}

static void CharCallback(GLFWwindow* window, unsigned int codepoint)
{
	// ImGui_ImplGlfw_CharCallback(window, codepoint);

	(void)window;

	InputEvent event;
	event.m_type      = InputEventType::Text;
	event.m_codepoint = codepoint;
	AddInputEvent(event);
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	// ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);

	InputEvent event;
	event.m_type      = InputEventType::MouseWheel;
	event.m_position  = GetMousePosition(window);
	event.m_scroll    = Vector2(static_cast<float>(xoffset), static_cast<float>(yoffset));
	event.m_modifiers = GetMouseModifiers(window);
	AddInputEvent(event);
}

static void WindowFocusCallback(GLFWwindow* window, int focused)
{
	(void)window;
	s_hasPreviousMousePosition = false;

	InputEvent event;
	event.m_type    = InputEventType::WindowFocus;
	event.m_focused = focused == GLFW_TRUE;
	AddInputEvent(event);
}

static void CursorEnterCallback(GLFWwindow* window, int entered)
{
	InputEvent event;
	event.m_type     = InputEventType::CursorEnter;
	event.m_position = GetMousePosition(window);
	event.m_entered  = entered == GLFW_TRUE;
	AddInputEvent(event);
}

static void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	(void)window;

	EventArgs args;
	args.SetValue("width", std::to_string(width));
	args.SetValue("height", std::to_string(height));
	FireEvent("WindowResized", args);
}
} // namespace

WindowSystem::WindowSystem(WindowSystemConfig config) : m_config(config) {}

WindowSystem::~WindowSystem() {}

void WindowSystem::BindMethods() {}

void WindowSystem::Startup()
{
	if (!glfwInit())
	{
		ERROR_AND_DIE("Failed to initialize GLFW");
	}

	CreateGLFWWindow();
}

void WindowSystem::Shutdown()
{
	if (m_glfwWindow != nullptr)
	{
		glfwDestroyWindow(m_glfwWindow);
		m_glfwWindow = nullptr;
	}

	glfwTerminate();
}

void WindowSystem::BeginFrame() { RunMessagePump(); }

void WindowSystem::EndFrame() {}

void WindowSystem::SetCursorMode(CursorMode mode)
{
	if (m_glfwWindow == nullptr || (mode != CursorMode::POINTER && mode != CursorMode::FPS))
	{
		return;
	}

	int const nativeMode = mode == CursorMode::FPS ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL;
	if (glfwGetInputMode(m_glfwWindow, GLFW_CURSOR) == nativeMode)
	{
		return;
	}

	s_hasPreviousMousePosition = false;
	s_previousCursorMode = nativeMode;
	glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, nativeMode);
}

void WindowSystem::CreateGLFWWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

	GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
	GLFWvidmode const* videoMode      = primaryMonitor != nullptr ? glfwGetVideoMode(primaryMonitor) : nullptr;

	m_glfwWindow = glfwCreateWindow(videoMode->width, videoMode->height, m_config.m_appName.c_str(), nullptr, nullptr);

	if (m_glfwWindow == nullptr)
	{
		ERROR_AND_DIE("Failed to create GLFW window");
	}

	s_previousMousePosition    = Vector2::Zero;
	s_hasPreviousMousePosition = false;
	s_previousCursorMode       = glfwGetInputMode(m_glfwWindow, GLFW_CURSOR);

	glfwSetKeyCallback(m_glfwWindow, KeyCallback);
	glfwSetMouseButtonCallback(m_glfwWindow, MouseButtonCallback);
	glfwSetCursorPosCallback(m_glfwWindow, CursorPosCallback);
	glfwSetCharCallback(m_glfwWindow, CharCallback);
	glfwSetScrollCallback(m_glfwWindow, ScrollCallback);
	glfwSetWindowFocusCallback(m_glfwWindow, WindowFocusCallback);
	glfwSetCursorEnterCallback(m_glfwWindow, CursorEnterCallback);
	glfwSetFramebufferSizeCallback(m_glfwWindow, FramebufferSizeCallback);

	glfwMaximizeWindow(m_glfwWindow);
	glfwShowWindow(m_glfwWindow);

	HWND hWnd      = glfwGetWin32Window(m_glfwWindow);
	m_windowHandle = static_cast<void*>(hWnd);
}

void WindowSystem::RunMessagePump()
{
	glfwPollEvents();

	if (m_glfwWindow != nullptr && glfwWindowShouldClose(m_glfwWindow))
	{
		FireEvent("Quit");
	}
}

void* WindowSystem::GetHwnd() const { return m_windowHandle; }

GLFWwindow* WindowSystem::GetGLFWWindow() const { return m_glfwWindow; }

IntVec2 WindowSystem::GetClientDimensions() const
{
	if (m_glfwWindow == nullptr)
	{
		return IntVec2::Zero;
	}

	int width  = 0;
	int height = 0;
	glfwGetFramebufferSize(m_glfwWindow, &width, &height);
	return IntVec2(width, height);
}
