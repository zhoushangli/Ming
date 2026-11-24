#pragma once

#include "MingEngine/Engine/Application/SystemBase.hpp"

#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/String.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include "MingEngine/Engine/Input/InputEvent.hpp"

struct GLFWwindow;
struct GLFWcursor;
enum class CursorMode;

#include <string>
#include <functional>
#include <vector>

struct WindowSystemConfig
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
	WindowSystem(WindowSystemConfig config);
	~WindowSystem();

	void SetFilesDroppedCallback(std::function<void(std::vector<std::string> const&, Vector2)> callback);
	void HandleFilesDropped(int count, char const** paths);

	void Startup() override;
	void Shutdown() override;

	void BeginFrame() override;
	void EndFrame() override;

	GLFWwindow* GetGLFWWindow() const;
	void*       GetHwnd() const;
	IntVec2     GetClientDimensions() const;
	void        SetCursorMode(CursorMode mode);
	void        SetCursorShape(CursorShape shape);
	bool        TryGetScreenColor(Vector2 clientPosition, Color& outColor) const;
	CursorShape GetCursorShape() const { return m_cursorShape; }
	String      GetClipboardText() const;
	bool        SetClipboardText(String const& text);
	void        SetTitleBarColors(Color const& background, Color const& text);

protected:
	static void BindMethods();

private:
	void CreateGLFWWindow();
	void RunMessagePump();

public:
	void*       m_windowHandle = 0;
	GLFWwindow* m_glfwWindow   = nullptr;

private:
	std::function<void(std::vector<std::string> const&, Vector2)> m_filesDroppedCallback;
	WindowSystemConfig m_config;
	GLFWcursor*        m_resizeHorizontalCursor = nullptr;
	GLFWcursor*        m_resizeVerticalCursor   = nullptr;
	GLFWcursor*        m_textCursor             = nullptr;
	GLFWcursor*        m_crosshairCursor        = nullptr;
	CursorShape        m_cursorShape            = CursorShape::Arrow;
};
