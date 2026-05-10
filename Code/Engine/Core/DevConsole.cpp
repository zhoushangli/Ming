#include "Engine/Core/DevConsole.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Renderer/Renderer.hpp"

Rgba8 const DevConsole::kError               = Rgba8(255, 0, 0, 255);     // Red
Rgba8 const DevConsole::kWarning             = Rgba8(255, 255, 0, 255);   // Yellow
Rgba8 const DevConsole::kInfoMajor           = Rgba8(0, 255, 0, 255);     // Green
Rgba8 const DevConsole::kInfoMinor           = Rgba8(0, 255, 255, 255);   // Cyan
Rgba8 const DevConsole::kInputText           = Rgba8(255, 255, 255, 255); // White
Rgba8 const DevConsole::kInputInsertionPoint = Rgba8(255, 255, 255, 255); // White

DevConsole::DevConsole(DevConsoleConfig const& config) : m_config(config), m_uiCamera() {}

DevConsole::~DevConsole() {}

void DevConsole::Startup()
{
	m_lines.clear();
	m_commandHistory.clear();

	m_isOpen = m_config.m_startOpen;

	Vec2 screenSize = (Vec2)g_engine->m_window->GetClientDimensions();
	m_uiCamera.SetOrthographicView(Vec2::Zero, screenSize, 0.f, 1.f);

	m_inputText.clear();
	m_insertionPointPosition = 0;
	m_insertionPointVisible  = true;

	m_insertionPointBlinkTimer = new Timer(0.5f, &Clock::GetSystemClock());
	m_insertionPointBlinkTimer->Start();
	m_insertionPointVisible = true;

	AddLine(kInfoMajor, "DevConsole started");

	g_engine->m_eventSystem->SubscribeEventCallbackFunction("KeyDown", DevConsole::Event_KeyDown);
	g_engine->m_eventSystem->SubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);

	g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_Quit", DevConsole::Command_Quit);
	g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_Clear", DevConsole::Command_Clear);
	g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_Help", DevConsole::Command_Help);
}

void DevConsole::Shutdown()
{
	g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_Help", DevConsole::Command_Help);
	g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_Clear", DevConsole::Command_Clear);
	g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_Quit", DevConsole::Command_Quit);

	g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);
	g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("KeyDown", DevConsole::Event_KeyDown);

	m_lines.clear();
	m_commandHistory.clear();

	if (m_insertionPointBlinkTimer)
	{
		delete m_insertionPointBlinkTimer;
		m_insertionPointBlinkTimer = nullptr;
	}
}

void DevConsole::BeginFrame() {}

void DevConsole::EndFrame() {}

void DevConsole::Execute(std::string const& consoleContext, bool echoCommand)
{
	// consoleContext example: "MyCommand arg1=value1 arg2=value2"

	Strings spaceTokens = SplitStringOnDelimiter(consoleContext, ' ');

	std::string commandName = Stringf("Dev_%s", spaceTokens[0].c_str());

	EventArgs args;
	for (int i = 1; i < (int)spaceTokens.size(); ++i)
	{
		Strings keyValue = SplitStringOnDelimiter(spaceTokens[i], '=');

		if (keyValue.size() == 2)
		{
			args.SetValue(keyValue[0], keyValue[1]);
		}
	}

	if (g_engine->m_eventSystem->IsEventRegistered(commandName))
	{
		if (echoCommand)
		{
			AddLine(kInputText, consoleContext);
		}
		m_commandHistory.push_back(consoleContext);
		g_engine->m_eventSystem->FireEvent(commandName, args);
	}
	else
	{
		AddLine(kError, Stringf("Unknown command: %s", consoleContext.c_str()));
		return;
	}
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text)
{
	DevConsoleLine line;
	line.m_color = color;
	line.m_text  = text;
	m_lines.push_back(line);
}

void DevConsole::Render()
{
	if (!m_isOpen)
		return;

	AABB2 const uiBounds = m_uiCamera.GetOrthographicBounds();

	g_engine->m_renderer->BeginCamera(m_uiCamera);
	g_engine->m_renderer->BindTexture(nullptr);
	g_engine->m_renderer->BindSampler(SamplerMode::POINT_CLAMP);
	g_engine->m_renderer->BindShader(nullptr);
	g_engine->m_renderer->BindModelConstants(Matrix4x4::Identity, Rgba8::White);

	g_engine->m_renderer->SetBlendMode(BlendMode::ALPHA);
	g_engine->m_renderer->SetDepthMode(DepthMode::READ_ONLY_ALWAYS);
	g_engine->m_renderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);

	std::vector<Vertex> bgVerts;
	AddVertsForAABB2D(bgVerts, uiBounds, Rgba8::TranslucentBlack);

	g_engine->m_renderer->DrawVertexArray((int)bgVerts.size(), bgVerts.data());

	std::string const fontFullPath = m_config.m_fontPath + "/" + m_config.m_fontName;
	BitmapFont*       font         = g_engine->m_renderer->CreateOrGetBitmapFont(fontFullPath.c_str());

	float const cellHeight     = uiBounds.GetDimensions().y / (float)m_config.m_linesOnScreen;
	int const   maxLinesToDraw = m_config.m_linesOnScreen - 1; // reserve one line for input

	// Flatten lines by '\n'
	struct VisualLine
	{
		Rgba8       color;
		std::string text;
		bool        isFirstInGroup = true;
	};

	std::vector<VisualLine> visualLines;
	visualLines.reserve(m_lines.size());

	for (DevConsoleLine const& line : m_lines)
	{
		std::string const& full = line.m_text;

		Strings parts = SplitStringOnDelimiter(full, '\n');
		for (size_t i = 0; i < parts.size(); ++i)
		{
			std::string part = parts[i];

			for (size_t tabPos = part.find('\t'); tabPos != std::string::npos; tabPos = part.find('\t', tabPos + 4))
			{
				part.replace(tabPos, 1, "    ");
			}

			visualLines.push_back(VisualLine{line.m_color, part, i == 0});
		}
	}

	int const numVisualLines = (int)visualLines.size();
	int const linesToDraw    = std::min(numVisualLines, maxLinesToDraw);

	std::vector<Vertex> textVerts;

	// Draw lines from bottom up
	for (int i = 0; i < linesToDraw; ++i)
	{
		int               visualIdx = numVisualLines - i - 1;
		VisualLine const& vline     = visualLines[visualIdx];

		std::string prefix = vline.isFirstInGroup ? "> " : "  ";
		std::string text   = prefix + vline.text;

		float y = uiBounds.m_mins.y + cellHeight * (i + 1);

		AABB2 lineBox(Vec2(uiBounds.m_mins.x, y), Vec2(uiBounds.m_maxs.x, y + cellHeight));

		font->AddVertsForTextInBox2D(
			textVerts,
			text,
			lineBox,
			cellHeight * 0.9f,
			vline.color,
			m_config.m_fontAspect,
			Vec2(0.f, 0.5f),
			TextBoxMode::SHRINK_TO_FIT
		);
	}

	// Input line
	AABB2 inputBox(Vec2(uiBounds.m_mins.x, uiBounds.m_mins.y), Vec2(uiBounds.m_maxs.x, uiBounds.m_mins.y + cellHeight));

	// Insertion point blink
	if (m_insertionPointBlinkTimer != nullptr)
	{
		while (m_insertionPointBlinkTimer->DecrementPeriodIfElapsed())
		{
			m_insertionPointVisible = !m_insertionPointVisible;
		}
	}

	// Draw input text normally
	std::string const prompt = "> " + m_inputText;

	font->AddVertsForTextInBox2D(
		textVerts,
		prompt,
		inputBox,
		cellHeight * 0.9f,
		kInputText,
		m_config.m_fontAspect,
		Vec2(0.f, 0.5f),
		TextBoxMode::SHRINK_TO_FIT
	);

	if (m_insertionPointVisible)
	{
#pragma region Insertion Point Rendering

		float const inputCellHeight = cellHeight * 0.9f;

		std::string const promptPrefix = "> ";
		int const         clampedPos   = GetClamped(m_insertionPointPosition, 0, (int)m_inputText.size());

		std::string const leftOfCursor = promptPrefix + m_inputText.substr(0, clampedPos);

		float const leftWidth = font->GetTextWidth(inputCellHeight, leftOfCursor, m_config.m_fontAspect);

		float const fullPromptWidth = font->GetTextWidth(inputCellHeight, prompt, m_config.m_fontAspect);
		float const boxWidth        = inputBox.GetDimensions().x;

		float textStartX = inputBox.m_mins.x;
		if (fullPromptWidth < boxWidth)
		{
			textStartX = inputBox.m_mins.x;
		}

		float const cursorGlyphWidth = font->GetTextWidth(inputCellHeight, "|", m_config.m_fontAspect);

		float const cursorCenterX = textStartX + leftWidth;
		float const cursorMinsX   = cursorCenterX - (cursorGlyphWidth * 0.5f);
		float const cursorMaxsX   = cursorCenterX + (cursorGlyphWidth * 0.5f);

		AABB2 cursorBox(Vec2(cursorMinsX, inputBox.m_mins.y), Vec2(cursorMaxsX, inputBox.m_maxs.y));

		font->AddVertsForTextInBox2D(
			textVerts,
			"|",
			cursorBox,
			inputCellHeight,
			kInputInsertionPoint,
			m_config.m_fontAspect,
			Vec2(0.5f, 0.5f),
			TextBoxMode::SHRINK_TO_FIT
		);

#pragma endregion
	}

	g_engine->m_renderer->BindTexture(font->GetTexture());
	g_engine->m_renderer->BindSampler(SamplerMode::POINT_CLAMP);
	g_engine->m_renderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

void DevConsole::ToggleOpen()
{
	m_isOpen = !m_isOpen;

	if (m_isOpen)
	{
		m_insertionPointVisible = true;
		m_insertionPointBlinkTimer->Start();
	}
	else
	{
		m_inputText.clear();
		m_insertionPointPosition = 0;
		m_commandHistory.clear();
		m_historyIndex = 0;
	}
}

bool DevConsole::IsOpen() { return m_isOpen; }

void DevConsole::HandleInput(unsigned char asKey)
{
	m_insertionPointPosition = GetClamped(m_insertionPointPosition, 0, (int)m_inputText.size());

	m_inputText.insert(m_inputText.begin() + m_insertionPointPosition, (char)asKey);
	++m_insertionPointPosition;

	m_insertionPointPosition = GetClamped(m_insertionPointPosition, 0, (int)m_inputText.size());
}

bool DevConsole::Event_KeyDown(EventArgs& args)
{
	unsigned char asKey = (unsigned char)std::stoi(args.GetValue("asKey", "0"));

	if (asKey == KeyCodeTilde)
	{
		g_engine->m_devConsole->ToggleOpen();
		return true;
	}

	if (g_engine->m_devConsole == nullptr || !g_engine->m_devConsole->IsOpen())
		return false;

	if (asKey == KeyCodeEnter)
	{
		std::string command = g_engine->m_devConsole->m_inputText;
		g_engine->m_devConsole->m_inputText.clear();
		g_engine->m_devConsole->m_insertionPointPosition = 0;

		if (command.empty())
		{
			g_engine->m_devConsole->ToggleOpen();
		}
		else
		{
			g_engine->m_devConsole->Execute(command, true);
		}

		g_engine->m_devConsole->m_insertionPointVisible = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeEsc)
	{
		if (g_engine->m_devConsole->m_inputText.empty())
		{
			g_engine->m_devConsole->ToggleOpen();
		}
		else
		{
			g_engine->m_devConsole->m_inputText.clear();
			g_engine->m_devConsole->m_insertionPointPosition = 0;
		}

		g_engine->m_devConsole->m_insertionPointVisible = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeHome)
	{
		g_engine->m_devConsole->m_insertionPointPosition = 0;
		g_engine->m_devConsole->m_insertionPointVisible  = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeEnd)
	{
		g_engine->m_devConsole->m_insertionPointPosition = (int)g_engine->m_devConsole->m_inputText.size();
		g_engine->m_devConsole->m_insertionPointVisible  = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeDelete)
	{
		int&         pos  = g_engine->m_devConsole->m_insertionPointPosition;
		std::string& text = g_engine->m_devConsole->m_inputText;

		pos = GetClamped(pos, 0, (int)text.size());
		if (pos < (int)text.size())
		{
			text.erase(text.begin() + pos);
		}

		g_engine->m_devConsole->m_insertionPointVisible = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeBackspace)
	{
		int&         pos  = g_engine->m_devConsole->m_insertionPointPosition;
		std::string& text = g_engine->m_devConsole->m_inputText;

		pos = GetClamped(pos, 0, (int)text.size());
		if (pos > 0 && !text.empty())
		{
			text.erase(text.begin() + (pos - 1));
			--pos;
		}

		g_engine->m_devConsole->m_insertionPointVisible = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeLeftArrow)
	{
		g_engine->m_devConsole->m_insertionPointPosition =
			std::max(0, g_engine->m_devConsole->m_insertionPointPosition - 1);
		g_engine->m_devConsole->m_insertionPointVisible = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeRightArrow)
	{
		g_engine->m_devConsole->m_insertionPointPosition = std::min(
			(int)g_engine->m_devConsole->m_inputText.size(),
			g_engine->m_devConsole->m_insertionPointPosition + 1
		);
		g_engine->m_devConsole->m_insertionPointVisible = true;
		g_engine->m_devConsole->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeUpArrow)
	{
		DevConsole* dc           = g_engine->m_devConsole;
		int const   historyCount = (int)dc->m_commandHistory.size();
		if (historyCount > 0)
		{
			dc->m_historyIndex--;
			if (dc->m_historyIndex < 0)
			{
				dc->m_historyIndex = historyCount - 1;
			}

			dc->m_inputText              = dc->m_commandHistory[dc->m_historyIndex];
			dc->m_insertionPointPosition = (int)dc->m_inputText.size();
		}

		dc->m_insertionPointVisible = true;
		dc->m_insertionPointBlinkTimer->Start();
		return true;
	}

	if (asKey == KeyCodeDownArrow)
	{
		DevConsole* dc           = g_engine->m_devConsole;
		int const   historyCount = (int)dc->m_commandHistory.size();
		if (historyCount > 0)
		{
			dc->m_historyIndex++;
			if (dc->m_historyIndex >= historyCount)
			{
				dc->m_historyIndex = 0;
			}

			dc->m_inputText              = dc->m_commandHistory[dc->m_historyIndex];
			dc->m_insertionPointPosition = (int)dc->m_inputText.size();
		}

		dc->m_insertionPointVisible = true;
		dc->m_insertionPointBlinkTimer->Start();
		return true;
	}

	return true;
}

bool DevConsole::Event_CharInput(EventArgs& args)
{
	if (g_engine->m_devConsole == nullptr || !g_engine->m_devConsole->IsOpen())
		return false;

	unsigned char asKey = (unsigned char)std::stoi(args.GetValue("asKey", "0"));

	if (asKey >= 32 && asKey <= 126)
	{
		if (asKey == '`' || asKey == '~')
		{
			return true;
		}

		g_engine->m_devConsole->HandleInput(asKey);
	}

	return true;
}

bool DevConsole::Command_Quit([[maybe_unused]] EventArgs& args)
{
	FireEvent("Quit");
	return true;
}

bool DevConsole::Command_Clear([[maybe_unused]] EventArgs& args)
{
	GUARANTEE_OR_DIE(g_engine->m_devConsole != nullptr, "DevConsole::Command_Clear called but g_theDevConsole is null");

	g_engine->m_devConsole->m_lines.clear();
	return true;
}

bool DevConsole::Command_Help([[maybe_unused]] EventArgs& args)
{
	GUARANTEE_OR_DIE(g_engine->m_devConsole != nullptr, "DevConsole::Command_Help called but g_theDevConsole is null");

	Strings eventNames = g_engine->m_eventSystem->GetRegisteredEventNames();

	std::string helpText;
	helpText.reserve(256);

	helpText += "Registered commands:";

	static std::string const kDevPrefix = "Dev_";
	for (std::string const& name : eventNames)
	{
		if (name.rfind(kDevPrefix, 0) != 0)
		{
			continue;
		}

		helpText += "\n\t";
		helpText += name.substr(kDevPrefix.size());
	}

	g_engine->m_devConsole->AddLine(DevConsole::kInfoMinor, helpText);
	return true;
}
