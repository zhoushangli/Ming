#pragma once

#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"

#include <string>

struct DevConsoleConfig
{
    bool m_isEnable = true;
};

struct DevConsoleLine
{
    Rgba8       m_color;
    std::string m_text;
    int         m_frameNumber = 0;
};

enum class DevConsoleMode
{
    HIDDEN,
    OPEN_FULL
};

class DevConsole
{
public:
    DevConsole(DevConsoleConfig const& config);
    ~DevConsole();

    void Startup();
    void Shutdown();
    void BeginFrame();
    void EndFrame();

    void Execute(std::string const& consoleCommandText);
    void AddLine(Rgba8 const& color, std::string const& text);
    void Render(AABB2 const& bounds) const;

    DevConsoleMode GetMode() const;
    void SetMode(DevConsoleMode mode);
    void ToggleMode(DevConsoleMode mode);

    static const Rgba8 ERROR;
    static const Rgba8 WARNING;
    static const Rgba8 INFO_MAJOR;
    static const Rgba8 INFO_MINOR;

protected:
    void Render_OpenFull(AABB2 const& bounds, BitmapFont& font, float fontAspect = 1.f) const;

protected:
    DevConsoleConfig       m_config;
    DevConsoleMode         m_mode = DevConsoleMode::HIDDEN;   // also OPEN_FULL, and eventually others
    std::vector<DevConsoleLine> m_lines;                      // #ToDo: support a max limited # of lines (e.g. fixed circular buffer)
    int                    m_frameNumber = 0;
};
