#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <string>

struct EditorUIContext;

class CreateScenePopup
{
public:
	void Open(VirtualPath const& parentVirtualPath);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath);

private:
	bool ValidateEntryName(char const* name, std::string& outError) const;

private:
	char        m_sceneName[256] = {};
	char        m_rootName[256] = {};
	bool        m_openRequested = false;
	bool        m_focusNameInput = false;
	VirtualPath m_parentVirtualPath;
	VirtualPath m_selectedVirtualPath;
	std::string m_error;
};
