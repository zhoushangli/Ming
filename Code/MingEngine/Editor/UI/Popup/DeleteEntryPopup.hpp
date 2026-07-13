#pragma once

#include <string>

struct EditorUIContext;

class DeleteEntryPopup
{
public:
	void Open(std::string const& targetVirtualPath, bool isDirectory);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(std::string& outVirtualPath);

private:
	bool        m_openRequested = false;
	bool        m_isDirectory = false;
	std::string m_parentVirtualPath;
	std::string m_targetVirtualPath;
	std::string m_selectedVirtualPath;
	std::string m_error;
};
