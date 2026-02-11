#include "Engine/Core/DevConsole.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Core/Timer.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/MathUtils.hpp"

#include <algorithm>

Rgba8 const DevConsole::ERROR                 = Rgba8(255, 0, 0, 255);      // Red
Rgba8 const DevConsole::WARNING               = Rgba8(255, 255, 0, 255);    // Yellow
Rgba8 const DevConsole::INFO_MAJOR            = Rgba8(0, 255, 0, 255);      // Green
Rgba8 const DevConsole::INFO_MINOR            = Rgba8(0, 255, 255, 255);    // Cyan
Rgba8 const DevConsole::INPUT_TEXT            = Rgba8(255, 255, 255, 255);  // White
Rgba8 const DevConsole::INPUT_INSERTION_POINT = Rgba8(255, 255, 255, 255);  // White

DevConsole::DevConsole(DevConsoleConfig const& config)
    : m_config(config)
{
}

DevConsole::~DevConsole()
{
}

void DevConsole::Startup()
{
    m_lines.clear();
    m_commandHistory.clear();

    m_isOpen = m_config.m_startOpen;

    m_inputText.clear();
    m_insertionPointPosition = 0;
    m_insertionPointVisible = true;

    if (m_insertionPointBlinkTimer == nullptr)
    {
        m_insertionPointBlinkTimer = new Timer(0.1f, &Clock::GetSystemClock());
    }

    AddLine(INFO_MINOR, "DevConsole started");

    g_engine->m_eventSystem->SubscribeEventCallbackFunction("KeyDown", DevConsole::Event_KeyDown);
    g_engine->m_eventSystem->SubscribeEventCallbackFunction("CharInput", DevConsole::Event_CharInput);

    g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_Clear", DevConsole::Command_Clear);
    g_engine->m_eventSystem->SubscribeEventCallbackFunction("Dev_Help", DevConsole::Command_Help);
}

void DevConsole::Shutdown()
{
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_Help", DevConsole::Command_Help);
    g_engine->m_eventSystem->UnsubscribeEventCallbackFunction("Dev_Clear", DevConsole::Command_Clear);

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

void DevConsole::BeginFrame()
{
    // Blink insertion point (simple toggle; uses timer if your Timer supports it)
    // If Timer is not fully implemented yet, keep it always visible.
    if (m_insertionPointBlinkTimer)
    {
        // Expected Timer API in many codebases: Start(), HasPeriodElapsed(), etc.
        // To avoid depending on unknown API, do a conservative no-op here.
        // m_insertionPointVisible = ...
    }
}

void DevConsole::EndFrame()
{
}

void DevConsole::Execute(std::string const& consoleContext, bool echoCommand)
{
    // consoleContext example: "MyCommand arg1=value1 arg2=value2"

    if (echoCommand)
    {
        AddLine(INPUT_TEXT, consoleContext);
    }

    Strings spaceTokens = SplitStringOnDelimiter(consoleContext, ' ');

    std::string commandName = spaceTokens[0];

    EventArgs args;
    for (int i = 1; i < (int)spaceTokens.size(); ++i)
    {
        Strings keyValue = SplitStringOnDelimiter(spaceTokens[i], '=');

        if (keyValue.size() == 2)
        {
            args.SetValue(keyValue[0], keyValue[1]);
        }
    }

    g_engine->m_eventSystem->FireEvent(commandName, args);
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text)
{
    DevConsoleLine line;
    line.m_color = color;
    line.m_text = text;
    m_lines.push_back(line);
}

void DevConsole::Render(AABB2 const& bounds)
{
    if (!m_isOpen)
        return;

    std::vector<Vertex> bgVerts;
    AddVertsForAABB2D(bgVerts, bounds, Rgba8::TRANSLUCENT_BLACK);

    m_config.m_renderer->BindTexture(nullptr);
    m_config.m_renderer->DrawVertexArray((int)bgVerts.size(), bgVerts.data());

    BitmapFont* font = m_config.m_renderer->CreateOrGetBitmapFont(m_config.m_fontName.c_str());

    float const cellHeight = bounds.GetDimensions().y / (float)m_config.m_linesOnScreen;
    int const maxLinesToDraw = m_config.m_linesOnScreen - 1; // reserve one line for input

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
        size_t start = 0;
        bool firstInThisLine = true;

        while (true)
        {
            size_t pos = full.find('\n', start);
            std::string part = full.substr(start, pos - start);

            VisualLine v;
            v.color = line.m_color;
            v.text = part;
            v.isFirstInGroup = firstInThisLine;
            visualLines.push_back(v);

            if (pos == std::string::npos)
                break;

            start = pos + 1;
            firstInThisLine = false;
        }
    }

    int const numVisualLines = (int)visualLines.size();
    int const linesToDraw = std::min(numVisualLines, maxLinesToDraw);

    std::vector<Vertex> textVerts;

    // Draw lines from bottom up (above input)
    for (int i = 0; i < linesToDraw; ++i)
    {
        int visualIdx = numVisualLines - i - 1;
        VisualLine const& vline = visualLines[visualIdx];

        std::string prefix = vline.isFirstInGroup ? "> " : "     ";
        std::string text = prefix + vline.text;

        float y = bounds.m_mins.y + cellHeight * (i + 1);

        AABB2 lineBox(
            Vec2(bounds.m_mins.x, y),
            Vec2(bounds.m_maxs.x, y + cellHeight)
        );

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

    // Input line (bottom)
    AABB2 inputBox(
        Vec2(bounds.m_mins.x, bounds.m_mins.y),
        Vec2(bounds.m_maxs.x, bounds.m_mins.y + cellHeight)
    );

    std::string prompt = "> " + m_inputText;

    // Insertion point visual (simple '|')
    if (m_insertionPointVisible)
    {
        int ip = std::clamp(m_insertionPointPosition, 0, (int)m_inputText.size());
        prompt = "> " + m_inputText.substr(0, (size_t)ip) + "|" + m_inputText.substr((size_t)ip);
    }

    font->AddVertsForTextInBox2D(
        textVerts,
        prompt,
        inputBox,
        cellHeight * 0.9f,
        INPUT_TEXT,
        m_config.m_fontAspect,
        Vec2(0.f, 0.5f),
        TextBoxMode::SHRINK_TO_FIT
    );

    m_config.m_renderer->BindTexture(&font->GetTexture());
    m_config.m_renderer->DrawVertexArray((int)textVerts.size(), textVerts.data());
}

void DevConsole::ToggleOpen()
{
    m_isOpen = !m_isOpen;
}

bool DevConsole::IsOpen()
{
    return m_isOpen;
}

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
    
    if (asKey == KEYCODE_TILDE)
    {
        g_engine->m_devConsole->ToggleOpen();
        return true;
    }

    if (g_engine->m_devConsole == nullptr || !g_engine->m_devConsole->IsOpen())
        return false;

    if (asKey == KEYCODE_ENTER)
    {
        std::string command = g_engine->m_devConsole->m_inputText;
        g_engine->m_devConsole->m_inputText.clear();
        g_engine->m_devConsole->m_insertionPointPosition = 0;

        if (!command.empty())
        {
            g_engine->m_devConsole->Execute(command, true);
        }
    }

    if (asKey == KEYCODE_ESC)
    {
        g_engine->m_devConsole->m_inputText.clear();
        g_engine->m_devConsole->m_insertionPointPosition = 0;
        g_engine->m_devConsole->ToggleOpen();
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
        g_engine->m_devConsole->HandleInput(asKey);
    }

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

    g_engine->m_devConsole->AddLine(INFO_MINOR, "Help: (listing commands not implemented here)");
    return true;
}

