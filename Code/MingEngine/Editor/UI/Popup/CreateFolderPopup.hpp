#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <string>

struct EditorUIContext;

class CreateFolderPopup
{
public:
	void Open(VirtualPath const& parentVirtualPath);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath);

private:
	bool ValidateName(EditorUIContext const& context, std::string& outError) const;

private:
	char        m_name[256] = {};
	bool        m_openRequested = false;
	bool        m_focusNameInput = false;
	VirtualPath m_parentVirtualPath;
	VirtualPath m_selectedVirtualPath;
	std::string m_error;
};
