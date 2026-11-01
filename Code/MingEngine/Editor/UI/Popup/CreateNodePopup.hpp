#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"

#include <map>
#include <string>
#include <vector>

struct EditorUIContext;
class Node;

class CreateNodePopup
{
public:
	void Open(ObjectID parentID);
	void Render(EditorUIContext& context);

private:
	void RebuildTypes();
	bool  CreateSelectedNode(EditorUIContext& context);
	Node* ResolveCreateParent(EditorUIContext const& context) const;
	void  Reset();

private:
	EditorUIContext m_context;
	PopupPanel* m_popup = nullptr;
	Tree* m_types = nullptr;
	Button* m_createButton = nullptr;
	LineEdit* m_search = nullptr;
	std::map<uint64_t, std::string> m_typeNames;
	std::string m_selectedClass;
	ObjectID    m_parentID      = ObjectID::Invalid;
	bool        m_openRequested = false;
};
