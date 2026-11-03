#pragma once

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

#include <string>

class ProjectSettingsPopup
{
public:
	~ProjectSettingsPopup();
	void               Open();
	bool               Confirm();
	void               Cancel();
	std::string const& GetError() const { return m_error; }
	void               Render(EditorUIContext& context);
	void               PrepareFields(EditorUIContext& context);
	Control*           GetContentControl() const { return m_content; }
	bool               IsOpen() const;

private:
	bool           m_openRequested = false;
	std::string    m_startScenePath;
	std::string    m_error;
	PopupPanel*    m_popup      = nullptr;
	VBoxContainer* m_content    = nullptr;
	Label*         m_errorLabel = nullptr;
	ObjectID       m_editID     = ObjectID::Invalid;
};
