#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"
#include "MingEngine/Scene/Core/NodeHandle.hpp"

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
	NodeHandle m_renamingNode = NodeHandle::Invalid;
	std::string m_originalName;
	bool m_focusRenameInput = false;
	CreateNodePopup m_createNodePopup;

	struct PendingReparent
	{
		void Clear()
		{
			m_child  = NodeHandle::Invalid;
			m_parent = NodeHandle::Invalid;
		}

		NodeHandle m_child;
		NodeHandle m_parent;
	};
	PendingReparent m_pendingReparent;
};
