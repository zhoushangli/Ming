#pragma once

#include <string>

struct EditorUIContext;

class CreateScenePopup
{
public:
	void Open(std::string const& parentVirtualPath);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(std::string& outVirtualPath);

private:
	bool ValidateEntryName(char const* name, std::string& outError) const;

private:
	char        m_sceneName[256] = {};
	char        m_rootName[256] = {};
	bool        m_openRequested = false;
	bool        m_focusNameInput = false;
	std::string m_parentVirtualPath;
	std::string m_selectedVirtualPath;
	std::string m_error;
};
