#include "Engine/Core/DevConsole.hpp"

#include "Engine/Core/Engine.hpp"

// Static color definitions
Rgba8 const DevConsole::ERROR      = Rgba8(255, 0, 0, 255);      // Red
Rgba8 const DevConsole::WARNING    = Rgba8(255, 255, 0, 255);    // Yellow
Rgba8 const DevConsole::INFO_MAJOR = Rgba8(0, 255, 0, 255);      // Green
Rgba8 const DevConsole::INFO_MINOR = Rgba8(0, 255, 255, 255);    // Cyan

int const MAX_CONSOLE_LINES = 40;

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
    m_mode = DevConsoleMode::HIDDEN;
    m_frameNumber = 0;

    AddLine(ERROR, "This is an ERROR line");
    AddLine(WARNING, "This is a WARNING line");
    AddLine(INFO_MAJOR, "This is a INFO_MAJOR line");
    AddLine(INFO_MINOR, "This is a INFO_MINOR line");
}

void DevConsole::Shutdown()
{
    m_lines.clear();
}

void DevConsole::BeginFrame()
{
    ++m_frameNumber;
}

void DevConsole::EndFrame()
{
    // No-op for now
}

void DevConsole::Execute(std::string const& consoleCommandText)
{
    // For now, just echo the command as an info line
    AddLine(INFO_MAJOR, consoleCommandText);
}

void DevConsole::AddLine(Rgba8 const& color, std::string const& text)
{
    DevConsoleLine line;
    
    line.m_color = color;
    line.m_text = text;
    line.m_frameNumber = m_frameNumber;
    
    m_lines.push_back(line);
}

void DevConsole::Render(AABB2 const& bounds) const
{
    if (m_mode == DevConsoleMode::HIDDEN)
        return;

    std::vector<Vertex> verts;

    AddVertsForAABB2D(verts, bounds, Rgba8::TRANSLUCENT_BLACK);

    g_engine->m_renderer->BindTexture(nullptr);
    g_engine->m_renderer->DrawVertexArray((int)verts.size(), verts.data());

    switch (m_mode)
    {
        case DevConsoleMode::OPEN_FULL:
        {
            BitmapFont* font = g_engine->m_renderer->CreateOrGetBitmapFont("Data/Images/SquirrelFixedFont");
            Render_OpenFull(bounds, *font);
            break;
        }
          
        default:
            break;
    }
}

DevConsoleMode DevConsole::GetMode() const
{
    return m_mode;
}

void DevConsole::SetMode(DevConsoleMode mode)
{
    m_mode = mode;
}

void DevConsole::ToggleMode(DevConsoleMode mode)
{
    if (m_mode == mode)
    {
        m_mode = DevConsoleMode::HIDDEN;
    }
    else
    {
        m_mode = mode;
    }
}

bool DevConsole::Command_Test(EventArgs& args)
{
    std::string debugInfo = args.GetDebugInfo();
    g_engine->m_devConsole->AddLine(INFO_MAJOR, Stringf("Test command received:\n %s", debugInfo.c_str()));
    return false;
}

void DevConsole::Render_OpenFull(AABB2 const& bounds, BitmapFont& font, float fontAspect) const
{
    float cellHeight = (bounds.GetDimensions().y) / (float)MAX_CONSOLE_LINES;

    int maxLines = MAX_CONSOLE_LINES - 1; // reserve one line for input

    struct VisualLine
    {
        Rgba8       color;
        std::string text;
        bool        isFirstInGroup = true;
    };

    std::vector<VisualLine> visualLines;
    visualLines.reserve(m_lines.size());

    // Flatten DevConsoleLine into visual lines
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
            {
                break;
            }

            start = pos + 1;
            firstInThisLine = false;
        }
    }

    int numVisualLines = (int)visualLines.size();
    int linesToDraw = std::min(numVisualLines, maxLines);

    std::vector<Vertex> verts;

    // Draw visual lines from bottom up
    for (int i = 0; i < linesToDraw; ++i)
    {
        int visualIdx = numVisualLines - i - 1;
        VisualLine const& vline = visualLines[visualIdx];

        // First line of this DevConsoleLine uses "> ", others use tab
        std::string prefix = vline.isFirstInGroup ? "> " : "     ";
        std::string text = prefix + vline.text;

        float y = bounds.m_mins.y + cellHeight * (i + 1);

        AABB2 lineBox(
            Vec2(bounds.m_mins.x, y),
            Vec2(bounds.m_maxs.x, y + cellHeight)
        );

        font.AddVertsForTextInBox2D(
            verts,
            text,
            lineBox,
            cellHeight * 0.9f,
            vline.color,
            fontAspect,
            Vec2(0.f, 0.5f),
            TextBoxMode::SHRINK_TO_FIT
        );
    }

    // Input line
    AABB2 inputBox(
        Vec2(bounds.m_mins.x, bounds.m_mins.y),
        Vec2(bounds.m_maxs.x, bounds.m_mins.y + cellHeight)
    );

    bool showCursor = ((m_frameNumber / 30) % 2) == 0;
    std::string inputPrompt = showCursor ? "> " : "  ";

    font.AddVertsForTextInBox2D(
        verts,
        inputPrompt,
        inputBox,
        cellHeight * 0.9f,
        Rgba8::WHITE,
        fontAspect,
        Vec2(0.f, 0.5f),
        TextBoxMode::SHRINK_TO_FIT
    );

    g_engine->m_renderer->BindTexture(&font.GetTexture());
    g_engine->m_renderer->DrawVertexArray((int)verts.size(), verts.data());
}

