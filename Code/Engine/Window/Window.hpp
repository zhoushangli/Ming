#pragma once

#include <string>

struct WindowConfig
{
	bool			m_isEnable		= true;
	float			m_clientAspect	= 16.0f / 9.0f;
	std::string		m_appName		= "Unnamed SD Application";
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

private:
	void CreateOSWindow();
	void RunMessagePump();

public:
	void* m_displayDeviceContext = 0;

private:
	WindowConfig m_config;

};

