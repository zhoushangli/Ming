#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/GLFW/glfw3native.h"
#include "ThirdParty/imgui/backends/imgui_impl_glfw.h"

#pragma comment(lib, "ThirdParty/GLFW/glfw3.lib")

namespace
{
static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);

	(void)window;
	(void)scancode;
	(void)mods;

	if (key < 0)
	{
		return;
	}

	EventArgs args;
	args.SetValue("asKey", std::to_string(key));

	if (action == GLFW_PRESS)
	{
		FireEvent("KeyDown", args);
	}
	else if (action == GLFW_RELEASE)
	{
		FireEvent("KeyUp", args);
	}
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

	(void)window;
	(void)mods;

	// To simplify things, we'll treat mouse buttons as "keys" in our input system
	// And view them as just additional key codes that come after the last GLFW key code
	int keyCode = -1;
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		keyCode = KeyCodeLeftMouse;
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		keyCode = KeyCodeRightMouse;
	}

	if (keyCode < 0)
	{
		return;
	}

	if (action == GLFW_PRESS)
	{
		g_engine->m_inputSystem->HandleKeyPressed(keyCode);
	}
	else if (action == GLFW_RELEASE)
	{
		g_engine->m_inputSystem->HandleKeyReleased(keyCode);
	}
}

static void CharCallback(GLFWwindow* window, unsigned int codepoint)
{
	ImGui_ImplGlfw_CharCallback(window, codepoint);

	(void)window;

	if (codepoint > 255)
	{
		return;
	}

	EventArgs args;
	args.SetValue("asKey", std::to_string(codepoint));
	FireEvent("CharInput", args);
}

static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
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

WindowSystem::WindowSystem(WindowConfig config) : m_config(config) {}

WindowSystem::~WindowSystem() {}

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

	glfwSetKeyCallback(m_glfwWindow, KeyCallback);
	glfwSetMouseButtonCallback(m_glfwWindow, MouseButtonCallback);
	glfwSetCharCallback(m_glfwWindow, CharCallback);
	glfwSetScrollCallback(m_glfwWindow, ScrollCallback);
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
