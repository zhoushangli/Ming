#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

#include <string>

struct EditorUIContext;
class CreateNodePanel;
class Node;

class ScenePanel
{
public:
	ScenePanel();

	void Render(EditorUIContext& context, CreateNodePanel& createNodePanel);

	EditorPanel&       GetPanel();
	EditorPanel const& GetPanel() const;

private:
	void RenderNode(
		Node* node, std::string const& filterText, EditorUIContext& context, CreateNodePanel& createNodePanel);
	bool DoesNodeMatchFilter(Node const* node, std::string const& filterText) const;

private:
	EditorPanel m_panel;
	char        m_filter[64] = {};
};
