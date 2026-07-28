#include "MingEngine/Editor/UI/ScenePanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <cctype>
#include <cstring>

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
	if (context.m_sceneTree == nullptr
		|| (m_renamingNode.IsValid() && context.m_sceneTree->ResolveNode(m_renamingNode) == nullptr))
	{
		ClearRename();
	}

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
			m_createNodePopup.Open(NodeHandle::Invalid);
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
			if (selectedHandle == m_renamingNode)
			{
				ClearRename();
			}
			EditorNode::Get()->MarkSceneDirty();
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
	m_createNodePopup.Render(context);
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

	std::string const displayName       = node->GetName().empty() ? node->GetClassName() : node->GetName();
	bool const        isRenaming        = m_renamingNode == handle;
	bool const        isOpen            = ImGui::TreeNodeEx("##SceneNodeTree", flags);
	ImVec2 const      treeItemMin       = ImGui::GetItemRectMin();
	ImVec2 const      treeItemMax       = ImGui::GetItemRectMax();
	bool const        rowHovered        = ImGui::IsMouseHoveringRect(treeItemMin, treeItemMax);
	bool              treeClicked       = rowHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
	bool              treeDoubleClicked = rowHovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

	if (ImGui::BeginPopupContextItem("SceneNodeContext"))
	{
		if (context.m_selection != nullptr)
		{
			context.m_selection->SetSelected(handle);
		}
		if (ImGui::MenuItem("Add Child Node..."))
		{
			m_createNodePopup.Open(handle);
		}
		ImGui::EndPopup();
	}

	// Drag/drop must stay attached to the tree item, which has a stable ImGui ID.
	if (!isRenaming && ImGui::BeginDragDropSource())
	{
		EditorDragDrop& dragDrop = EditorNode::Get()->m_dragDrop;
		dragDrop.SetDragData(handle);
		ImGui::SetDragDropPayload(EditorDragDrop::PayloadType, nullptr, 0);
		ImGui::TextUnformatted(node->GetName().c_str());

		ImGui::EndDragDropSource();
	}

	if (!isRenaming && ImGui::BeginDragDropTarget())
	{
		EditorDragDrop& dragDrop = EditorNode::Get()->m_dragDrop;
		NodeHandle      draggedHandle;
		if (dragDrop.TryGetData(draggedHandle) && draggedHandle != handle)
		{
			dragDrop.AllowDrop();
			if (ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadType) != nullptr)
			{
				m_pendingReparent.m_child  = draggedHandle;
				m_pendingReparent.m_parent = handle;
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImVec2 const iconSize = EditorUIStyle::SceneTreeIconSize();

	// If the item is clicked, select the node
	if (isRenaming)
	{
		ImGui::SameLine();
		float const iconY = treeItemMin.y + (treeItemMax.y - treeItemMin.y - iconSize.y) * 0.5f;
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, iconY));
		EditorUIWidgets::RenderIcon(node->GetClassName(), Node::GetStaticClassName(), iconSize);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (m_focusRenameInput)
		{
			ImGui::SetKeyboardFocusHere();
			m_focusRenameInput = false;
		}

		bool const submitted = ImGui::InputText(
			"##NodeName",
			m_renameBuffer,
			sizeof(m_renameBuffer),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		bool const cancelled   = ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape);
		bool const deactivated = ImGui::IsItemDeactivated();
		if (cancelled)
		{
			FinishRename(node, false);
		}
		else if (submitted || deactivated)
		{
			FinishRename(node, true);
		}
	}
	else
	{
		EditorUIWidgets::RenderTreeRowContent(
			node->GetClassName(),
			Node::GetStaticClassName(),
			displayName,
			treeItemMin,
			treeItemMax,
			iconSize);
	}

	if (treeClicked && context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(handle);
	}

	if (treeDoubleClicked && !isRenaming)
	{
		BeginRename(node);
		if (context.m_selection != nullptr)
		{
			context.m_selection->SetSelected(handle);
		}
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

void ScenePanel::BeginRename(Node* node)
{
	if (node == nullptr)
	{
		return;
	}

	m_renamingNode     = node->GetHandle();
	m_originalName     = node->GetName();
	m_focusRenameInput = true;
	strncpy_s(m_renameBuffer, m_originalName.c_str(), sizeof(m_renameBuffer) - 1);
}

void ScenePanel::FinishRename(Node* node, bool apply)
{
	if (node != nullptr)
	{
		node->SetName(apply ? m_renameBuffer : m_originalName);
	}
	ClearRename();
}

void ScenePanel::ClearRename()
{
	m_renamingNode = NodeHandle::Invalid;
	m_originalName.clear();
	m_renameBuffer[0]  = '\0';
	m_focusRenameInput = false;
}
