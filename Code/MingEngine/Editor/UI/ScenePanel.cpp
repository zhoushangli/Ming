#include "MingEngine/Editor/UI/ScenePanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/CreateNodePanel.hpp"
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
	std::transform(lowerText.begin(),
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

ScenePanel::ScenePanel()
	: m_panel("Scene")
{
}

void ScenePanel::Render(EditorUIContext& context, CreateNodePanel& createNodePanel)
{
	if (!m_panel.IsOpen())
	{
		return;
	}

	ImGui::Begin(m_panel.GetTitle(), m_panel.GetOpenState());
	ImGui::InputTextWithHint("##FilterNodes", "Filter Nodes", m_filter, sizeof(m_filter));
	ImGui::Separator();

	Node* sceneRoot = context.m_sceneTree != nullptr ? context.m_sceneTree->GetScene() : nullptr;
	if (sceneRoot != nullptr)
	{
		RenderNode(sceneRoot, m_filter, context, createNodePanel);
	}

	if (ImGui::BeginPopupContextWindow(
			"ScenePanelContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("Add Child Node..."))
		{
			createNodePanel.Open(NodeHandle::Invalid);
		}
		ImGui::EndPopup();
	}

	ImGui::End();
}

EditorPanel& ScenePanel::GetPanel() { return m_panel; }

EditorPanel const& ScenePanel::GetPanel() const { return m_panel; }

void ScenePanel::RenderNode(
	Node* node, std::string const& filterText, EditorUIContext& context, CreateNodePanel& createNodePanel)
{
	if (node == nullptr || !node->IsSerializable() || !DoesNodeMatchFilter(node, filterText))
	{
		return;
	}

	bool hasVisibleChildren = false;
	for (Node const* child : node->GetChildren())
	{
		if (child != nullptr && child->IsSerializable() && DoesNodeMatchFilter(child, filterText))
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
	if (context.m_selection != nullptr && context.m_selection->GetSelectedNodeHandle() == node->GetHandle())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	NodeHandle const handle = node->GetHandle();
	ImGui::PushID(static_cast<int>(handle.GetUID()));
	ImGui::PushID(static_cast<int>(handle.GetIndex()));

	std::string const displayName = node->GetName().empty() ? node->GetClassName() : node->GetName();
	bool const        isOpen      = ImGui::TreeNodeEx(displayName.c_str(), flags);
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(handle);
	}

	if (ImGui::BeginPopupContextItem("SceneNodeContext"))
	{
		if (context.m_selection != nullptr)
		{
			context.m_selection->SetSelected(handle);
		}
		if (ImGui::MenuItem("Add Child Node..."))
		{
			createNodePanel.Open(handle);
		}
		ImGui::EndPopup();
	}

	if (isOpen && hasVisibleChildren)
	{
		for (Node* child : node->GetChildren())
		{
			RenderNode(child, filterText, context, createNodePanel);
		}
		ImGui::TreePop();
	}

	ImGui::PopID();
	ImGui::PopID();
}

bool ScenePanel::DoesNodeMatchFilter(Node const* node, std::string const& filterText) const
{
	if (node == nullptr || !node->IsSerializable())
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
