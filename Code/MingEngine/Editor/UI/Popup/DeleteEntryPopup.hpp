#pragma once

#include "MingEngine/Core/IO/VirtualPath.hpp"

#include <string>
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

struct EditorUIContext;

class DeleteEntryPopup
{
public:
	void Open(VirtualPath const& targetVirtualPath, bool isDirectory);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath);

private:
	void Confirm();

private:
	EditorUIContext m_context;
	PopupPanel*     m_popup         = nullptr;
	LineEdit*       m_nameEdit      = nullptr;
	LineEdit*       m_rootEdit      = nullptr;
	Label*          m_errorLabel    = nullptr;
	bool            m_openRequested = false;
	bool            m_isDirectory   = false;
	VirtualPath     m_parentVirtualPath;
	VirtualPath     m_targetVirtualPath;
	VirtualPath     m_selectedVirtualPath;
	std::string     m_error;
};
