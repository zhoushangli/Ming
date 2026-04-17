#pragma once

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Camera.hpp"

#include <string>
#include <vector>

class Renderer;
class Camera;
class BitmapFont;
class Timer;

// Stores the text and color for an individual line of text
struct DevConsoleLine
{
	Rgba8       m_color;
	std::string m_text;
};

// Dev console defaults. A Renderer and Camera must be provided.
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

// Class for a dev console that allows entering text and executing commands. Can be toggled with
// tilde ('~') and renders within a transparent box with configurable bounds. Other features
// include specific coloring for different lines of text and a blinking insertion point.
class DevConsole
{
public:
	DevConsole(DevConsoleConfig const& config);
	~DevConsole();

	// Subscribes to any events needed, prints an initial line of text, and starts the blink timer.
	void Startup();
	void Shutdown();
	void BeginFrame();
	void EndFrame();

	// Parses the current input line and executes it using the event system. Commands and arguments
	// are delimited from each other with space (' ') and argument names and values are delimited
	// with equals ('='). Echoes the command to the dev console as well as any command output.
	void Execute(std::string const& consoleContext, bool echoCommand = true);

	// Adds a line of text to the current list of lines being shown. Individual lines are delimited
	// with the newline ('\n') character.
	void AddLine(Rgba8 const& color, std::string const& text);

	// Renders just visible text lines within the bounds specified. Bounds are in terms of the
	// camera being used to render. The current input line renders at the bottom with all other
	// lines rendered above it, with the most recent lines at the bottom.
	void Render();

	// Toggles between open and closed.
	void ToggleOpen();
	bool IsOpen();

	void HandleInput(unsigned char asKey);

	static const Rgba8 ERROR;
	static const Rgba8 WARNING;
	static const Rgba8 INFO_MAJOR;
	static const Rgba8 INFO_MINOR;
	static const Rgba8 INPUT_TEXT;
	static const Rgba8 INPUT_INSERTION_POINT;

	// Handle key input.
	static bool Event_KeyDown(EventArgs& args);

	// Handle char input by appending valid characters to our current input line.
	static bool Event_CharInput(EventArgs& args);

	static bool Command_Quit(EventArgs& args);

	// Clear all lines of text.
	static bool Command_Clear(EventArgs& args);

	// Display all currently registered commands in the event system.
	static bool Command_Help(EventArgs& args);

protected:
	DevConsoleConfig m_config;
	Camera           m_uiCamera;

	// True if the dev console is currently visible and accepting input.
	bool m_isOpen = false;

	// All lines added to the dev console since the last time it was cleared.
	std::vector<DevConsoleLine> m_lines;

	// Our current line of input text.
	std::string m_inputText;

	// Index of the insertion point in our current input text.
	int m_insertionPointPosition = 0;

	// True if our insertion point is currently in the visible phase of blinking.
	bool m_insertionPointVisible = true;

	// Timer for controlling insertion point visibility.
	Timer* m_insertionPointBlinkTimer;

	// History of all commands executed.
	std::vector<std::string> m_commandHistory;

	// Our current index in our history of commands as we are scrolling.
	int m_historyIndex = -1;
};
