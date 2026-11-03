#pragma once

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"

#include <map>
#include <string>
#include <vector>

class CreateNodePopup
{
public:
	void Open(ObjectID parentID);
	void Render(EditorUIContext& context);
	void SetCreateCallback(std::function<bool(std::string const&, std::string&)> callback);

private:
	void RebuildTypes();
	void RefreshSelection();
	void RebuildSidebar();
	void Confirm();
	void LoadState();
	void SaveState();
	void HandleKey(InputEvent const& event);

private:
	EditorUIContext                                       m_context;
	PopupPanel*                                           m_popup        = nullptr;
	Tree*                                                 m_types        = nullptr;
	VBoxContainer*                                        m_sidebar      = nullptr;
	Button*                                               m_createButton = nullptr;
	Button*                                               m_favorite     = nullptr;
	LineEdit*                                             m_search       = nullptr;
	Label*                                                m_description  = nullptr;
	Label*                                                m_error        = nullptr;
	Label*                                                m_parentLabel  = nullptr;
	std::map<uint64_t, std::string>                       m_typeNames;
	std::vector<std::string>                              m_favorites;
	std::vector<std::string>                              m_recent;
	std::string                                           m_selectedClass;
	ObjectID                                              m_parentID      = ObjectID::Invalid;
	bool                                                  m_openRequested = false;
	std::function<bool(std::string const&, std::string&)> m_createCallback;
};
