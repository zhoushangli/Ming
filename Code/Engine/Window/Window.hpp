#pragma once

#include "Engine/Math/Vec2.hpp"

#include <string>

struct WindowConfig
{
	bool        m_isEnable     = true;
	float       m_clientAspect = 16.0f / 9.0f;
	Vec2        m_resolution   = Vec2(1920.f, 1080.f);
	std::string m_appName      = "Unnamed SD Application";
};

class Window
{
public:
	Window(WindowConfig config);
	~Window();

	void Startup();
	void Shutdown();

	void BeginFrame();
	void EndFrame();

	Vec2 GetNormalizedMouseUV() const;

	void*   GetHwnd() const;
	IntVec2 GetClientDimensions() const;

private:
	void CreateOSWindow();
	void RunMessagePump();

public:
	void* m_windowHandle         = 0;
	void* m_displayDeviceContext = 0;

private:
	WindowConfig m_config;
};
