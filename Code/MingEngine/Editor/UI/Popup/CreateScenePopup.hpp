#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <string>
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

struct EditorUIContext;

class CreateScenePopup
{
public:
	void Open(VirtualPath const& parentVirtualPath);
	void Render(EditorUIContext& context);
	bool ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath);

private:
	void Confirm();
	bool ValidateEntryName(char const* name, std::string& outError) const;

private:
	EditorUIContext m_context;
	PopupPanel*     m_popup      = nullptr;
	LineEdit*       m_nameEdit   = nullptr;
	LineEdit*       m_rootEdit   = nullptr;
	Label*          m_errorLabel = nullptr;
	Button*         m_confirm    = nullptr;
	VirtualPath     m_overwrite;
	char            m_sceneName[256] = {};
	char            m_rootName[256]  = {};
	bool            m_openRequested  = false;
	bool            m_focusNameInput = false;
	VirtualPath     m_parentVirtualPath;
	VirtualPath     m_selectedVirtualPath;
	std::string     m_error;
};
