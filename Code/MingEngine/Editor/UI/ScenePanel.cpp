#include "MingEngine/Editor/UI/ScenePanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/CreateNodePanel.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <cctype>

namespace
{
std::string ToLower(std::string const& text)
{
	std::string lowerText = text;
	std::transform(
		lowerText.begin(),
		lowerText.end(),
		lowerText.begin(),
		[](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return lowerText;
}

bool ContainsCaseInsensitive(std::string const& text, std::string const& filterText)
{
	return filterText.empty() || ToLower(text).find(ToLower(filterText)) != std::string::npos;
}
} // namespace

ScenePanel::ScenePanel() : EditorPanel("Scene") {}

void ScenePanel::OnRender(EditorUIContext& context)
{
	m_pendingReparent.Clear();

	ImGui::Begin(GetTitle(), GetOpenState());
	ImGui::InputTextWithHint("##FilterNodes", "Filter Nodes", m_filter, sizeof(m_filter));
	ImGui::Separator();

	Node* sceneRoot = context.m_sceneTree != nullptr ? context.m_sceneTree->GetScene() : nullptr;
	if (sceneRoot != nullptr)
	{
		RenderNode(sceneRoot, m_filter, context);
	}

	// If the user right-clicks on the window, show the context menu
	if (ImGui::BeginPopupContextWindow(
			"ScenePanelContext",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Add Child Node..."))
		{
			CreateNodePanelData data;
			data.m_parentHandle = NodeHandle::Invalid;
			context.m_editorUI->OpenPanel<CreateNodePanel>(data);
		}
		ImGui::EndPopup();
	}

	// If the user clicks on the window without hovering over any item, clear the selection
	if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()
		&& context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(NodeHandle::Invalid);
	}

	// If we want to delete the selected node
	if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && context.m_selection != nullptr)
	{
		NodeHandle const selectedHandle = context.m_selection->GetSelected();
		Node*            selectedNode   = context.m_sceneTree->ResolveNode(selectedHandle);

		if (selectedNode)
		{
			selectedNode->DeleteNode();
		}
	}

	// If we want to reparent a node
	if (m_pendingReparent.m_child != NodeHandle::Invalid && m_pendingReparent.m_parent != NodeHandle::Invalid)
	{
		Node* childNode  = context.m_sceneTree->ResolveNode(m_pendingReparent.m_child);
		Node* parentNode = context.m_sceneTree->ResolveNode(m_pendingReparent.m_parent);

		if (childNode && parentNode)
		{
			childNode->Reparent(parentNode, true);
		}
		m_pendingReparent.Clear();
	}

	ImGui::End();
}

void ScenePanel::RenderNode(Node* node, std::string const& filterText, EditorUIContext& context)
{
	if (node == nullptr || !DoesNodeMatchFilter(node, filterText))
	{
		return;
	}

	bool hasVisibleChildren = false;
	for (Node const* child : node->GetChildren())
	{
		if (child != nullptr && DoesNodeMatchFilter(child, filterText))
		{
			hasVisibleChildren = true;
			break;
		}
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!hasVisibleChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (!filterText.empty())
	{
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	if (context.m_selection != nullptr && context.m_selection->GetSelected() == node->GetHandle())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	NodeHandle const handle = node->GetHandle();
	ImGui::PushID(static_cast<int>(handle.GetUID()));
	ImGui::PushID(static_cast<int>(handle.GetIndex()));

	std::string const displayName = node->GetName().empty() ? node->GetClassName() : node->GetName();
	bool const        isOpen      = ImGui::TreeNodeEx(displayName.c_str(), flags);

	// If the item is clicked, select the node
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(handle);
	}

	// If the item is dragged, start a drag and drop operation
	if (ImGui::BeginDragDropSource())
	{
		ImGui::SetDragDropPayload("SCENE_NODE", &handle, sizeof(handle));

		ImGui::Text("%s", displayName.c_str());
		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget())
	{
		if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("SCENE_NODE"))
		{
			NodeHandle const draggedHandle = *static_cast<NodeHandle const*>(payload->Data);

			m_pendingReparent.m_child  = draggedHandle;
			m_pendingReparent.m_parent = handle;
		}

		ImGui::EndDragDropTarget();
	}

	if (ImGui::BeginPopupContextItem("SceneNodeContext"))
	{
		if (context.m_selection != nullptr)
		{
			context.m_selection->SetSelected(handle);
		}
		if (ImGui::MenuItem("Add Child Node..."))
		{
			CreateNodePanelData data;
			data.m_parentHandle = handle;
			context.m_editorUI->OpenPanel<CreateNodePanel>(data);
		}
		ImGui::EndPopup();
	}

	if (isOpen && hasVisibleChildren)
	{
		for (Node* child : node->GetChildren())
		{
			RenderNode(child, filterText, context);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
	ImGui::PopID();
}

bool ScenePanel::DoesNodeMatchFilter(Node const* node, std::string const& filterText) const
{
	if (node == nullptr)
	{
		return false;
	}

	std::string const displayName = node->GetName().empty() ? node->GetClassName() : node->GetName();
	if (ContainsCaseInsensitive(displayName, filterText))
	{
		return true;
	}
	for (Node const* child : node->GetChildren())
	{
		if (DoesNodeMatchFilter(child, filterText))
		{
			return true;
		}
	}
	return false;
}
