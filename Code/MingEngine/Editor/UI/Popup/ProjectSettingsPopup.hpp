#pragma once

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"

#include <string>

class ProjectSettingsPopup
{
public:
	void               Open();
	bool               Confirm();
	void               Cancel();
	std::string const& GetError() const { return m_error; }
	void               Render(EditorUIContext& context);
	void               PrepareFields(EditorUIContext& context);
	Control*           GetContentControl() const { return m_content; }
	bool               IsOpen() const;
	bool               HasChanges() const;

private:
	void RefreshAutoloads();
	void Browse(EditorUIContext const& context, bool autoload);
	void AddAutoload(FileSystem const& fileSystem);
	void SetError(std::string const& error);

private:
	bool                                  m_openRequested    = false;
	bool                                  m_refreshAutoloads = false;
	std::string                           m_startScenePath;
	std::vector<ProjectSettings::AutoLoad> m_autoLoads;
	std::string                           m_error;

	PopupPanel*                           m_popup                = nullptr;
	VBoxContainer*                        m_content              = nullptr;
	Label*                                m_errorLabel           = nullptr;
	LineEdit*                             m_mainScene            = nullptr;
	LineEdit*                             m_filter               = nullptr;
	LineEdit*                             m_autoloadPath         = nullptr;
	LineEdit*                             m_autoloadName         = nullptr;
	VBoxContainer*                        m_autoloadRows         = nullptr;
	VBoxContainer*                        m_autoloadEmpty        = nullptr;
	Label*                                m_autoloadNameTitle    = nullptr;
	Label*                                m_autoloadActionsTitle = nullptr;
};
