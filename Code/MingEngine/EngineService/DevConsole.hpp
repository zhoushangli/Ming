#pragma once

#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"

#include <array>
#include <map>
#include <string>
#include <vector>

struct ImGuiInputTextCallbackData;

struct DevConsoleLine
{
	Rgba8       m_color;
	std::string m_text;
};

struct DevConsoleConfig
{
	bool m_isEnable = true;

	std::string m_fontPath = "Data/Fonts";
	std::string m_fontName = "SquirrelFixedFont";

	float m_fontAspect        = 1.f;
	int   m_linesOnScreen     = 32;
	int   m_maxCommandHistory = 128;
	bool  m_startOpen         = false;
};

using DevConsoleCommandFunc = bool (*)(EventArgs& args);

class DevConsole
{
public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole();

	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	void AddCommand(std::string const& name, DevConsoleCommandFunc func);
	void Execute(std::string const& consoleContext, bool echoCommand = true);
	void AddLine(Rgba8 const& color, std::string const& text);
	void Render();

	void ToggleOpen();
	bool IsOpen() const;

	static const Rgba8 kError;
	static const Rgba8 kWarning;
	static const Rgba8 kInfoMajor;
	static const Rgba8 kInfoMinor;
	static const Rgba8 kInputText;

	static bool Quit(EventArgs& args);
	static bool Clear(EventArgs& args);
	static bool Help(EventArgs& args);

private:
	struct CommandEntry
	{
		std::string           m_name;
		DevConsoleCommandFunc m_func = nullptr;
	};

	static int InputTextCallbackStub(ImGuiInputTextCallbackData* data);
	int        InputTextCallback(ImGuiInputTextCallbackData* data);
	void       ExecuteInputBuffer();
	void       ResetInputBuffer();
	void       RecallHistory(int direction);
	bool       IsRuntimeEnabled() const;

private:
	DevConsoleConfig                    m_config;
	bool                                m_isOpen              = false;
	bool                                m_focusInputNextFrame = false;
	bool                                m_scrollToBottom      = false;
	std::vector<DevConsoleLine>         m_lines;
	std::map<std::string, CommandEntry> m_commands;
	std::vector<std::string>            m_commandHistory;
	int                                 m_historyIndex = -1;
	std::array<char, 512>               m_inputBuffer{};
};
