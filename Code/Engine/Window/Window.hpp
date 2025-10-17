#pragma once

class Window
{
public:
	Window();
	~Window();

	void Startup();
	void Shutdown();

	void BeginFrame();
	void EndFrame();
};

