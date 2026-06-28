#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Math/Vec2.hpp"

struct GLFWwindow;

#include <string>

struct WindowConfig
{
	bool        m_isEnable               = true;
	float       m_clientAspect           = 16.0f / 9.0f;
	std::string m_appName                = "Unnamed SD Application";
	bool        m_isFullscreen           = false;
	bool        m_isResizable            = false;
	bool        m_isBorderlessFullscreen = true;
};

class WindowSystem : public SystemBase
{
	MCLASS(WindowSystem, SystemBase)

public:
	WindowSystem(WindowConfig config);
	~WindowSystem();

	void Startup() override;
	void Shutdown() override;

	void BeginFrame() override;
	void EndFrame() override;

	GLFWwindow* GetGLFWWindow() const;
	void*       GetHwnd() const;
	IntVec2     GetClientDimensions() const;

protected:
	static void BindMethods();

private:
	void CreateGLFWWindow();
	void RunMessagePump();

public:
	void*       m_windowHandle = 0;
	GLFWwindow* m_glfwWindow   = nullptr;

private:
	WindowConfig m_config;
};
