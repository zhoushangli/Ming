#include "Engine/Window/Window.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

Window::Window(WindowConfig config) : m_config(config)
{

}

Window::~Window()
{

}

void Window::Startup()
{
	CreateOSWindow();
}

void Window::Shutdown()
{

}

void Window::BeginFrame()
{
	RunMessagePump();
}

void Window::EndFrame()
{

}

Vec2 Window::GetNormalizedMouseUV() const
{
    HWND windowHandle = static_cast<HWND>(m_windowHandle); // Need to add this new void* member!
    POINT cursorCoords;
    RECT clientRect;

    ::GetCursorPos(&cursorCoords);                         // in Windows screen coordinates; (0,0) is top-left
    ::ScreenToClient(windowHandle, &cursorCoords);          // get relative to this window's client area
    ::GetClientRect(windowHandle, &clientRect);             // dimensions of client area (0,0 to width,height)
    float cursorX = static_cast<float>(cursorCoords.x) / static_cast<float>(clientRect.right);
    float cursorY = static_cast<float>(cursorCoords.y) / static_cast<float>(clientRect.bottom);
    return Vec2(cursorX, 1.f - cursorY);                    // Flip Y; we want (0,0) bottom-left, not top-left
}

LRESULT CALLBACK WindowsMessageHandlingProcedure(HWND windowHandle, UINT wmMessageCode, WPARAM wParam, LPARAM lParam)
{
	switch (wmMessageCode)
	{
	case WM_CLOSE:
	{
		FireEvent("Quit");
		// return 0;
	}

	case WM_KEYDOWN:
	{
		if (g_engine == nullptr || g_engine->m_input == nullptr)
		{
			break;
		}

		unsigned char asKey = (unsigned char)wParam;
		g_engine->m_input->HandleKeyPressed(asKey);

		break;
	}

	case WM_KEYUP:
	{
		if (g_engine == nullptr || g_engine->m_input == nullptr)
		{
			break;
		}

		unsigned char asKey = (unsigned char)wParam;
		g_engine->m_input->HandleKeyReleased(asKey);

		break;
	}

	case WM_LBUTTONDOWN:
	{
		if (g_engine == nullptr || g_engine->m_input == nullptr)
		{
			break;
		}

		g_engine->m_input->HandleKeyPressed(KEYCODE_LEFT_MOUSE);
		break;
	}

	case WM_LBUTTONUP:
	{
		if (g_engine == nullptr || g_engine->m_input == nullptr)
		{
			break;
		}

		g_engine->m_input->HandleKeyReleased(KEYCODE_LEFT_MOUSE);
		break;
	}

	case WM_RBUTTONDOWN:
	{
		if (g_engine == nullptr || g_engine->m_input == nullptr)
		{
			break;
		}
		g_engine->m_input->HandleKeyPressed(KEYCODE_RIGHT_MOUSE);
		break;
	}

	case WM_RBUTTONUP:
	{
		if (g_engine == nullptr || g_engine->m_input == nullptr)
		{
			break;
		}
		g_engine->m_input->HandleKeyReleased(KEYCODE_RIGHT_MOUSE);
		break;
	}
	}

	return DefWindowProc(windowHandle, wmMessageCode, wParam, lParam);
}

void Window::CreateOSWindow()
{
	HINSTANCE applicationInstanceHandle = ::GetModuleHandle(NULL);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Define a window style/class
	WNDCLASSEX windowClassDescription;
	memset(&windowClassDescription, 0, sizeof(windowClassDescription));
	windowClassDescription.cbSize = sizeof(windowClassDescription);
	windowClassDescription.style = CS_OWNDC; // Redraw on move, request own Display Context
	windowClassDescription.lpfnWndProc = static_cast<WNDPROC>(WindowsMessageHandlingProcedure); // Register our Windows message-handling function
	windowClassDescription.hInstance = applicationInstanceHandle;
	windowClassDescription.hIcon = NULL;
	windowClassDescription.hCursor = NULL;
	windowClassDescription.lpszClassName = TEXT("Simple Window Class");
	RegisterClassEx(&windowClassDescription);

	// #SD1ToDo: Add support for fullscreen mode (requires different window style flags than windowed mode)
	DWORD const windowStyleFlags = WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU | WS_OVERLAPPED;
	DWORD const windowStyleExFlags = WS_EX_APPWINDOW;

	// Get desktop rect, dimensions, aspect
	RECT desktopRect;
	HWND desktopWindowHandle = GetDesktopWindow();
	GetClientRect(desktopWindowHandle, &desktopRect);
	float desktopWidth = (float)(desktopRect.right - desktopRect.left);
	float desktopHeight = (float)(desktopRect.bottom - desktopRect.top);
	float desktopAspect = desktopWidth / desktopHeight;

	// Calculate maximum client size (as some % of desktop size)
	constexpr float maxClientFractionOfDesktop = 0.90f;
	float clientWidth = desktopWidth * maxClientFractionOfDesktop;
	float clientHeight = desktopHeight * maxClientFractionOfDesktop;
	if (m_config.m_clientAspect > desktopAspect)
	{
		// Client window has a wider aspect than desktop; shrink client height to match its width
		clientHeight = clientWidth / m_config.m_clientAspect;
	}
	else
	{
		// Client window has a taller aspect than desktop; shrink client width to match its height
		clientWidth = clientHeight * m_config.m_clientAspect;
	}

	// Calculate client rect bounds by centering the client area
	float clientMarginX = 0.5f * (desktopWidth - clientWidth);
	float clientMarginY = 0.5f * (desktopHeight - clientHeight);
	RECT clientRect;
	clientRect.left = (int)clientMarginX;
	clientRect.right = clientRect.left + (int)clientWidth;
	clientRect.top = (int)clientMarginY;
	clientRect.bottom = clientRect.top + (int)clientHeight;

	// Calculate the outer dimensions of the physical window, including frame et. al.
	RECT windowRect = clientRect;
	AdjustWindowRectEx(&windowRect, windowStyleFlags, FALSE, windowStyleExFlags);

	WCHAR windowTitle[1024];
	MultiByteToWideChar(GetACP(), 0, m_config.m_appName.c_str(), -1, windowTitle, sizeof(windowTitle) / sizeof(windowTitle[0]));
	
	HWND hWnd = CreateWindowEx(
		windowStyleExFlags,
		windowClassDescription.lpszClassName,
		windowTitle,
		windowStyleFlags,
		windowRect.left,
		windowRect.top,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		NULL,
		NULL,
		(HINSTANCE)applicationInstanceHandle,
		NULL);

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	SetFocus(hWnd);

	m_windowHandle = static_cast<void*>(hWnd);
	m_displayDeviceContext = GetDC(hWnd);

	HCURSOR cursor = LoadCursor(NULL, IDC_ARROW);
	SetCursor(cursor);
}

void Window::RunMessagePump()
{
	MSG queuedMessage;
	for (;; )
	{
		BOOL const wasMessagePresent = PeekMessage(&queuedMessage, NULL, 0, 0, PM_REMOVE);
		if (!wasMessagePresent)
		{
			break;
		}

		TranslateMessage(&queuedMessage);
		DispatchMessage(&queuedMessage); 
	}
}

