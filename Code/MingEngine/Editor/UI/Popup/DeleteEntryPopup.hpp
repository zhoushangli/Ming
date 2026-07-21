#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <string>

struct EditorUIContext;

class DeleteEntryPopup
{
public:
	void Open(VirtualPath const& targetVirtualPath, bool isDirectory);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath);

private:
	bool        m_openRequested = false;
	bool        m_isDirectory = false;
	VirtualPath m_parentVirtualPath;
	VirtualPath m_targetVirtualPath;
	VirtualPath m_selectedVirtualPath;
	std::string m_error;
};
