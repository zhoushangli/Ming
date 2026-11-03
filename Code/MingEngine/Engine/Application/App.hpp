#pragma once

#include "MingEngine/Engine/Event/EventSystem.hpp"

#include <filesystem>
#include <string>
#include <vector>

class Clock;
class EditorCamera;
class Label;
class Node;
class SceneTree;
class Control;
class LineEdit;
class Tree;
class PopupPanel;
class Button;

enum class MingRunMode
{
	ProjectManager,
	Editor,
	Game
};

struct MingRunConfig
{
	MingRunMode           mode = MingRunMode::ProjectManager;
	std::filesystem::path projectPath;

	// If true, the engine will generate C# bindings for all registered types and exit.
	bool                  generateCSharpBindings = false;
	std::filesystem::path csharpBindingsOutputDirectory;
};

namespace MingEngine
{
int Run(MingRunConfig const& config);
} // namespace MingEngine

class App
{
public:
	explicit App(MingRunConfig const& config);
	~App();

	void Startup();
	void Shutdown();
	void RunMainLoop();
	void RunFrame();
	void Restart();
	void Quit();

	bool        LaunchGame();
	bool        IsGameRunning() const { return m_gameProcess != nullptr; }
	void        StopGame();
	static bool CreateEmptyProject(std::filesystem::path const& directory, std::string& error);
	static std::vector<std::filesystem::path> LoadRecentProjects();
	static bool                               SaveRecentProjects(std::vector<std::filesystem::path> const& projects);
	static void                               RememberProject(std::filesystem::path const& directory);

	int        GetExitCode() const { return m_exitCode; }
	bool       IsQuitting() const { return m_shouldQuit; }
	SceneTree* GetSceneTree() const { return m_sceneTree; }

	static bool OnQuit(EventArgs& args);

private:
	void Update(float deltaSeconds);
	void PollGame();
	void Render() const;
	void BeginFrame();
	void EndFrame();
	void RestartImmediately();
	void ShutdownScene();

	void StartupEditor();
	void StartupGame();
	void DrawProjectManager();
	void OpenProject();
	bool OpenProject(std::filesystem::path const& directory);
	void NewProject();
	void RefreshRecentProjects();

private:
	void*              m_gameProcess      = nullptr;
	unsigned long      m_gameProcessID    = 0;
	unsigned long long m_gameStopDeadline = 0;
	bool               m_shouldRestart    = false;
	bool               m_shouldQuit       = false;
	int                m_exitCode         = 0;
	EventConnectionID  m_quitEventID      = 0;

	Clock*                             m_clock     = nullptr;
	SceneTree*                         m_sceneTree = nullptr;
	MingRunConfig                      m_runConfig;
	std::string                        m_projectManagerError;
	Label*                             m_projectManagerErrorLabel = nullptr;
	Control*                           m_projectManagerRoot       = nullptr;
	Tree*                              m_recentTree               = nullptr;
	LineEdit*                          m_projectSearch            = nullptr;
	std::vector<std::filesystem::path> m_recentProjects;
	PopupPanel*                        m_createProjectPopup = nullptr;
	LineEdit*                          m_projectName        = nullptr;
	LineEdit*                          m_projectParent      = nullptr;
	Label*                             m_createProjectError = nullptr;
	bool                               m_recentDirty        = true;

	EditorCamera*     m_editorCamera        = nullptr;
	bool              m_isSlowMode          = false;
	Button*           m_editProjectButton   = nullptr;
	Button*           m_removeProjectButton = nullptr;
	std::vector<bool> m_recentAvailable;
};

extern App* g_app;
