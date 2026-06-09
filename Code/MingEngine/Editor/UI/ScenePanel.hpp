#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
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

private:
	char m_filter[64] = {};

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
