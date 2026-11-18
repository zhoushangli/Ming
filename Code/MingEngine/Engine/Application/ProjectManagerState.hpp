#pragma once

#include "MingEngine/Engine/Application/AppState.hpp"

#include <filesystem>
#include <string>
#include <vector>

class Label;
class Control;
class LineEdit;
class Tree;
class PopupPanel;
class Button;

class ProjectManagerState : public AppState
{
public:
	bool Enter(App& app) override;
	void Exit(App& app) override;

	static bool CreateEmptyProject(std::filesystem::path const& directory, std::string& error);
	static std::vector<std::filesystem::path> LoadRecentProjects();
	static bool SaveRecentProjects(std::vector<std::filesystem::path> const& projects);
	static void RememberProject(std::filesystem::path const& directory);

private:
	void CreateProjectManager();
	void UpdateProjectManager();
	void OpenProject();
	bool OpenProject(std::filesystem::path const& directory);
	void NewProject();
	void RefreshRecentProjects();

private:
	App* m_app = nullptr;

	Control*    m_projectManagerRoot       = nullptr;
	std::string m_projectManagerError;
	Label*      m_projectManagerErrorLabel = nullptr;

	std::vector<std::filesystem::path> m_recentProjects;
	std::vector<bool>                  m_recentAvailable;
	bool                               m_recentDirty = true;

	Tree*     m_recentTree          = nullptr;
	LineEdit* m_projectSearch       = nullptr;
	Button*   m_editProjectButton   = nullptr;
	Button*   m_removeProjectButton = nullptr;

	PopupPanel* m_createProjectPopup = nullptr;
	LineEdit*   m_projectName        = nullptr;
	LineEdit*   m_projectParent      = nullptr;
	Label*      m_createProjectError = nullptr;
};
