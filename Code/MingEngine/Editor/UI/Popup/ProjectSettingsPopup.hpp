#pragma once

#include <string>
#include "MingEngine/Editor/UI/EditorControlHost.hpp"

struct EditorUIContext;

class ProjectSettingsPopup
{
public:
	~ProjectSettingsPopup();
	void                     Open();
	bool                     Confirm();
	void                     Cancel();
	std::string const&       GetError() const { return m_error; }
	void                     Render(EditorUIContext& context);
	void                     PrepareFields(EditorUIContext& context);
	EditorControlHost&       GetHost() { return m_host; }
	EditorControlHost const& GetHost() const { return m_host; }
	bool                     IsOpen() const { return m_isOpen; }

private:
	bool              m_openRequested = false;
	std::string       m_startScenePath;
	std::string       m_error;
	EditorControlHost m_host;
	ObjectID          m_editID = ObjectID::Invalid;
	bool              m_isOpen = false;
};
