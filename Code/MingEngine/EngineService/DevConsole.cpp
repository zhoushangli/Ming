#include "MingEngine/EngineService/DevConsole.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <cstring>

Rgba8 const DevConsole::kError     = Rgba8(255, 96, 96, 255);
Rgba8 const DevConsole::kWarning   = Rgba8(255, 220, 96, 255);
Rgba8 const DevConsole::kInfoMajor = Rgba8(96, 220, 255, 255);
Rgba8 const DevConsole::kInfoMinor = Rgba8(180, 190, 210, 255);
Rgba8 const DevConsole::kInputText = Rgba8(255, 255, 255, 255);

namespace
{
ImVec4 ToImGuiColor(Rgba8 const& color)
{
	return ImVec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, color.a / 255.f);
}
}

DevConsole::DevConsole(DevConsoleConfig const& config)
	: m_config(config)
{
	ResetInputBuffer();
}

DevConsole::~DevConsole() = default;

void DevConsole::Startup()
{
	if (!IsRuntimeEnabled())
	{
		return;
	}

	m_lines.clear();
	m_commands.clear();
	m_commandHistory.clear();
	m_historyIndex = -1;
	m_isOpen = m_config.m_startOpen;
	m_focusInputNextFrame = m_isOpen;
	m_scrollToBottom = m_isOpen;
	ResetInputBuffer();

	AddCommand("Quit", Command_Quit);
	AddCommand("Clear", Command_Clear);
	AddCommand("Help", Command_Help);
	AddLine(kInfoMajor, "DevConsole started");
}

void DevConsole::Shutdown()
{
	m_lines.clear();
	m_commands.clear();
	m_commandHistory.clear();
	m_historyIndex = -1;
	m_isOpen = false;
	m_focusInputNextFrame = false;
	m_scrollToBottom = false;
	ResetInputBuffer();
}

void DevConsole::BeginFrame()
{
	if (!IsRuntimeEnabled() || g_engine == nullptr || g_engine->m_input == nullptr)
	{
		return;
	}

	if (g_engine->m_input->WasKeyJustPressed(KeyCodeTilde))
	{
		ToggleOpen();
	}
}

void DevConsole::EndFrame() {}

void DevConsole::AddCommand(std::string const& name, DevConsoleCommandFunc func)
{
	if (name.empty() || func == nullptr)
	{
		return;
	}

	m_commands[name] = CommandEntry{ name, func };
}

void DevConsole::Execute(std::string const& consoleContext, bool echoCommand)
{
	Strings const rawTokens = SplitStringOnDelimiter(consoleContext, ' ');
	Strings tokens;
	tokens.reserve(rawTokens.size());
	for (std::string const& token : rawTokens)
	{
		if (!token.empty())
		{
			tokens.push_back(token);
		}
	}

	if (tokens.empty())
	{
		return;
	}

	auto iter = m_commands.find(tokens[0]);
	if (iter == m_commands.end())
	{
		AddLine(kError, Stringf("Unknown command: %s", tokens[0].c_str()));
		return;
	}

	if (echoCommand)
	{
		AddLine(kInputText, consoleContext);
	}

	EventArgs args;
	for (size_t i = 1; i < tokens.size(); ++i)
	{
		Strings const keyValue = SplitStringOnDelimiter(tokens[i], '=');
		if (keyValue.size() == 2 && !keyValue[0].empty())
		{
			args.SetValue(keyValue[0], keyValue[1]);
		}
	}

	if (m_commandHistory.empty() || m_commandHistory.back() != consoleContext)
	{
		m_commandHistory.push_back(consoleContext);
		if ((int)m_commandHistory.size() > m_config.m_maxCommandHistory)
		{
			m_commandHistory.erase(m_commandHistory.begin());
		}
	}

	m_historyIndex = static_cast<int>(m_commandHistory.size());
	iter->second.m_func(args);
	m_scrollToBottom = true;
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text)
{
	Strings const splitLines = SplitStringOnDelimiter(text, '\n');
	for (std::string const& splitLine : splitLines)
	{
		m_lines.push_back(DevConsoleLine{ color, splitLine });
	}
	m_scrollToBottom = true;
}

void DevConsole::Render()
{
	if (!IsRuntimeEnabled() || !m_isOpen)
	{
		return;
	}

	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImVec2 const windowPos(viewport->WorkPos.x, viewport->WorkPos.y);
	ImVec2 const windowSize(viewport->WorkSize.x, viewport->WorkSize.y * 0.42f);

	ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::SetNextWindowBgAlpha(0.92f);

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoSavedSettings;

	bool keepOpen = true;
	if (!ImGui::Begin("Dev Console", &keepOpen, windowFlags))
	{
		ImGui::End();
		if (!keepOpen)
		{
			ToggleOpen();
		}
		return;
	}

	if (!keepOpen)
	{
		ToggleOpen();
		ImGui::End();
		return;
	}

	ImGui::TextUnformatted("Enter command and press Enter");
	ImGui::Separator();

	float const footerHeight = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().WindowPadding.y;
	if (ImGui::BeginChild("DevConsoleLog", ImVec2(0.f, -footerHeight), true, ImGuiWindowFlags_HorizontalScrollbar))
	{
		for (DevConsoleLine const& line : m_lines)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ToImGuiColor(line.m_color));
			ImGui::TextWrapped("%s", line.m_text.c_str());
			ImGui::PopStyleColor();
		}

		if (m_scrollToBottom)
		{
			ImGui::SetScrollHereY(1.f);
			m_scrollToBottom = false;
		}
	}
	ImGui::EndChild();

	ImGuiInputTextFlags inputFlags = ImGuiInputTextFlags_EnterReturnsTrue;
	inputFlags |= ImGuiInputTextFlags_CallbackHistory;

	if (m_focusInputNextFrame)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusInputNextFrame = false;
	}

	bool const submitted = ImGui::InputText("##DevConsoleInput",
		m_inputBuffer.data(),
		m_inputBuffer.size(),
		inputFlags,
		InputTextCallbackStub,
		this);

	if (ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape, false))
	{
		if (m_inputBuffer[0] != '\0')
		{
			ResetInputBuffer();
		}
		else
		{
			ToggleOpen();
		}
	}

	if (submitted)
	{
		ExecuteInputBuffer();
	}

	ImGui::End();
}

void DevConsole::ToggleOpen()
{
	if (!IsRuntimeEnabled())
	{
		return;
	}

	m_isOpen = !m_isOpen;
	m_focusInputNextFrame = m_isOpen;
	m_scrollToBottom = m_isOpen;
	m_historyIndex = static_cast<int>(m_commandHistory.size());

	if (!m_isOpen)
	{
		ResetInputBuffer();
	}
}

bool DevConsole::IsOpen() const { return m_isOpen; }

bool DevConsole::Command_Quit([[maybe_unused]] EventArgs& args)
{
	FireEvent("Quit");
	return true;
}

bool DevConsole::Command_Clear([[maybe_unused]] EventArgs& args)
{
	GUARANTEE_OR_DIE(g_engineService != nullptr && g_engineService->m_console != nullptr,
		"DevConsole::Command_Clear called but console is null");
	g_engineService->m_console->m_lines.clear();
	g_engineService->m_console->m_scrollToBottom = true;
	return true;
}

bool DevConsole::Command_Help([[maybe_unused]] EventArgs& args)
{
	GUARANTEE_OR_DIE(g_engineService != nullptr && g_engineService->m_console != nullptr,
		"DevConsole::Command_Help called but console is null");

	std::string helpText = "Registered commands:";
	for (auto const& pair : g_engineService->m_console->m_commands)
	{
		helpText += "\n  ";
		helpText += pair.second.m_name;
	}

	g_engineService->m_console->AddLine(DevConsole::kInfoMinor, helpText);
	return true;
}

int DevConsole::InputTextCallbackStub(ImGuiInputTextCallbackData* data)
{
	DevConsole* console = static_cast<DevConsole*>(data->UserData);
	return console != nullptr ? console->InputTextCallback(data) : 0;
}

int DevConsole::InputTextCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory)
	{
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			RecallHistory(-1);
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			RecallHistory(1);
		}

		data->DeleteChars(0, data->BufTextLen);
		data->InsertChars(0, m_inputBuffer.data());
	}

	return 0;
}

void DevConsole::ExecuteInputBuffer()
{
	std::string const command = m_inputBuffer.data();
	ResetInputBuffer();

	if (command.empty())
	{
		ToggleOpen();
		return;
	}

	Execute(command, true);
	m_focusInputNextFrame = true;
}

void DevConsole::ResetInputBuffer()
{
	m_inputBuffer.fill('\0');
}

void DevConsole::RecallHistory(int direction)
{
	if (m_commandHistory.empty())
	{
		ResetInputBuffer();
		m_historyIndex = -1;
		return;
	}

	if (m_historyIndex < 0)
	{
		m_historyIndex = static_cast<int>(m_commandHistory.size());
	}

	m_historyIndex += direction;
	if (m_historyIndex < 0)
	{
		m_historyIndex = 0;
	}
	else if (m_historyIndex > (int)m_commandHistory.size())
	{
		m_historyIndex = static_cast<int>(m_commandHistory.size());
	}

	ResetInputBuffer();
	if (m_historyIndex >= 0 && m_historyIndex < (int)m_commandHistory.size())
	{
		strncpy_s(m_inputBuffer.data(), m_inputBuffer.size(), m_commandHistory[m_historyIndex].c_str(), _TRUNCATE);
	}
}

bool DevConsole::IsRuntimeEnabled() const
{
#if defined(MING_EDITOR)
	return false;
#else
	return true;
#endif
}


