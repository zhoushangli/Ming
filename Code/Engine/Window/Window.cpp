#include "Engine/Window/Window.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"

#define WIN32_LEAN_AND_MEAN
#define GLFW_EXPOSE_NATIVE_WIN32
#include "ThirdParty/GLFW/glfw3.h"
#include "ThirdParty/GLFW/glfw3native.h"

#pragma comment(lib, "ThirdParty/GLFW/glfw3.lib")

namespace
{
static void GLFWKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
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

static void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
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
		g_engine->m_input->HandleKeyPressed(keyCode);
	}
	else if (action == GLFW_RELEASE)
	{
		g_engine->m_input->HandleKeyReleased(keyCode);
	}
}

static void GLFWCharCallback(GLFWwindow* window, unsigned int codepoint)
{
	(void)window;

	if (codepoint > 255)
	{
		return;
	}

	EventArgs args;
	args.SetValue("asKey", std::to_string(codepoint));
	FireEvent("CharInput", args);
}
} // namespace

Window::Window(WindowConfig config) : m_config(config) {}

Window::~Window() {}

void Window::Startup()
{
	if (!glfwInit())
	{
		ERROR_AND_DIE("Failed to initialize GLFW");
	}

	CreateGLFWWindow();
}

void Window::Shutdown()
{
	if (m_glfwWindow != nullptr)
	{
		glfwDestroyWindow(m_glfwWindow);
		m_glfwWindow = nullptr;
	}

	glfwTerminate();
}

void Window::BeginFrame() { RunMessagePump(); }

void Window::EndFrame() {}

void Window::CreateGLFWWindow()
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
	GLFWvidmode const* videoMode      = primaryMonitor != nullptr ? glfwGetVideoMode(primaryMonitor) : nullptr;

	float desktopWidth  = videoMode != nullptr ? (float)videoMode->width : 1920.f;
	float desktopHeight = videoMode != nullptr ? (float)videoMode->height : 1080.f;

	// Calculate maximum client size (as some % of desktop size)
	float clientWidth  = desktopWidth * 0.8f;
	float clientHeight = desktopHeight * 0.8f;
	if (clientWidth / clientHeight > m_config.m_clientAspect)
	{
		clientWidth = clientHeight * m_config.m_clientAspect;
	}
	else
	{
		clientHeight = clientWidth / m_config.m_clientAspect;
	}
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);

	m_glfwWindow = glfwCreateWindow((int)clientWidth, (int)clientHeight, m_config.m_appName.c_str(), nullptr, nullptr);

	if (m_glfwWindow == nullptr)
	{
		ERROR_AND_DIE("Failed to create GLFW window");
	}

	glfwSetKeyCallback(m_glfwWindow, GLFWKeyCallback);
	glfwSetMouseButtonCallback(m_glfwWindow, GLFWMouseButtonCallback);
	glfwSetCharCallback(m_glfwWindow, GLFWCharCallback);

	glfwSetWindowPos(m_glfwWindow, (int)clientMarginX, (int)clientMarginY);
	glfwShowWindow(m_glfwWindow);

	HWND hWnd      = glfwGetWin32Window(m_glfwWindow);
	m_windowHandle = static_cast<void*>(hWnd);
}

void Window::RunMessagePump()
{
	glfwPollEvents();

	if (m_glfwWindow != nullptr && glfwWindowShouldClose(m_glfwWindow))
	{
		FireEvent("Quit");
	}
}

void* Window::GetHwnd() const { return m_windowHandle; }

GLFWwindow* Window::GetGLFWWindow() const { return m_glfwWindow; }

IntVec2 Window::GetClientDimensions() const
{
	if (m_glfwWindow == nullptr)
	{
		return IntVec2::Zero;
	}

	int width  = 0;
	int height = 0;
	glfwGetWindowSize(m_glfwWindow, &width, &height);
	return IntVec2(width, height);
}
