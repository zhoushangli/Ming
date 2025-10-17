#pragma once

struct WindowConfig
{
	bool m_isEnable = true;
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
	WindowConfig m_config;
};

