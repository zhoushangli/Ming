#pragma once

#include "MingEngine/Core/Object/ObjectID.hpp"
#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"

#include <string>

struct EditorUIContext;
class Node;

class ScenePanel final : public EditorPanel
{
public:
	ScenePanel();

private:
	void OnRender(EditorUIContext& context) override;
	void RenderNode(Node* node, std::string const& filterText, EditorUIContext& context);
	bool DoesNodeMatchFilter(Node const* node, std::string const& filterText) const;
	void BeginRename(Node* node);
	void FinishRename(Node* node, bool apply);
	void ClearRename();

private:
	char m_filter[64] = {};
	char m_renameBuffer[256] = {};
	ObjectID m_renamingNodeID = ObjectID::Invalid;
	std::string m_originalName;
	bool m_focusRenameInput = false;
	CreateNodePopup m_createNodePopup;

	struct PendingReparent
	{
		void Clear()
		{
			m_childID  = ObjectID::Invalid;
			m_parentID = ObjectID::Invalid;
		}

		ObjectID m_childID;
		ObjectID m_parentID;
	};
	PendingReparent m_pendingReparent;
};
