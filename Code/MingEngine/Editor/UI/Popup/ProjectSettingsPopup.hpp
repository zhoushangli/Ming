#pragma once

#include <string>

struct EditorUIContext;

class ProjectSettingsPopup
{
public:
	void Open();
	void Render(EditorUIContext& context);

private:
	bool        m_openRequested = false;
	std::string m_startScenePath;
	std::string m_error;
};
