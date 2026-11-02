#include "MingEngine/Editor/UI/ScenePanel.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cctype>

ScenePanel::ScenePanel() : EditorPanel("Scene")
{
}

void ScenePanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	layout->SetSeparation(static_cast<float>(GetThemeConstant("panel_margin", "Editor")));
	auto* margin = MemNew<MarginContainer>();
	margin->SetMargins(static_cast<float>(GetThemeConstant("panel_margin", "Editor")), static_cast<float>(GetThemeConstant("panel_margin", "Editor")), static_cast<float>(GetThemeConstant("panel_margin", "Editor")), static_cast<float>(GetThemeConstant("panel_margin", "Editor")));
	AddNode(margin);
	margin->AddNode(layout);
	layout->UpdateSize(GetSize());
	m_filter = MemNew<LineEdit>();
	m_filter->SetPlaceholder("Filter nodes");
	m_filter->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	m_filter->SetMinimumCharacterWidth(1);
	auto* tools = MemNew<HBoxContainer>();
	tools->SetSeparation(static_cast<float>(GetThemeConstant("panel_margin", "Editor")) * 0.5f);
	layout->AddNode(tools);
	Button* add = EditorPopupUtils::AddButton(tools, "", [this]() { OpenCreateNode(); });
	add->SetIcon(GetThemeIcon("Add", "EditorIcons"));
	add->SetThemeTypeVariation("EditorToolbarButton");
	m_filter->SetHorizontalSizeFlags(Size_ExpandFill);
	tools->AddNode(m_filter);
	m_empty = EditorPopupUtils::AddLabel(layout, "No scene open", "EditorStatusLabel");
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);
	m_tree = MemNew<Tree>();
	m_tree->SetClipText(true);
	scroll->AddNode(m_tree);
	m_tree->SetItemActionCallback([this](uint64_t key, int action)
	{
		auto found = m_nodeItems.find(key);
		auto* item = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<CanvasItem>(found->second) : nullptr;
		if (item != nullptr && !item->IsQueuedForDeletion()
			&& action == (item->GetScriptInstance() != nullptr ? 1 : 0))
		{
			item->SetVisible(!item->IsVisible());
			EditorNode::Get()->MarkSceneDirty();
		}
	});
	m_tree->SetSelectedCallback([this](uint64_t key)
	{
		auto found = m_nodeItems.find(key);
		m_context.m_selection->SetSelected(found != m_nodeItems.end() ? found->second : ObjectID::Invalid);
	});
	m_tree->SetRenamedCallback([this](uint64_t key, String const& name)
	{
		auto found = m_nodeItems.find(key);
		Node* node = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(found->second) : nullptr;
		if (node != nullptr && !node->IsQueuedForDeletion() && node->GetName() != name)
		{
			node->SetName(name);
			EditorNode::Get()->MarkSceneDirty();
		}
	});
	m_tree->SetInputCallback([this](InputEvent const& event) { return HandleTreeInput(event); });
	m_tree->SetDragCallbacks(
		[this](Vector2 const& position, GuiDragData& data)
		{
			auto found = m_nodeItems.find(m_tree->GetItemAtPosition(position));
			Node* node = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(found->second) : nullptr;
			if (node == nullptr || node->IsQueuedForDeletion() || node == m_context.m_sceneTree->GetScene())
			{
				return false;
			}

			data = { "node", node->GetObjectID(), node->GetName() };
			m_context.m_editorUI->SetNativeDragPreview(m_tree, node->GetName());
			return true;
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			return data.m_type == String("node") && CanReparent(data.m_objectID, m_tree->GetItemAtPosition(position));
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			uint64_t const key = m_tree->GetItemAtPosition(position);
			if (CanReparent(data.m_objectID, key))
			{
				Node* child = ObjectDatabase::GetInstance<Node>(data.m_objectID);
				child->Reparent(ObjectDatabase::GetInstance<Node>(m_nodeItems.at(key)), true);
				EditorNode::Get()->MarkSceneDirty();
			}
		});
}

void ScenePanel::OnRender(EditorUIContext& context)
{
	UpdateItems();
	m_createNodePopup.Render(context);
}

void ScenePanel::OpenCreateNode()
{
	m_createNodePopup.Open(m_context.m_selection->GetSelected());
}

void ScenePanel::DeleteSelected()
{
	Node* node = ObjectDatabase::GetInstance<Node>(m_context.m_selection->GetSelected());
	if (node != nullptr && node->GetSceneTree() == m_context.m_sceneTree && !node->IsQueuedForDeletion())
	{
		EditorNode::Get()->MarkSceneDirty();
		m_context.m_selection->Clear();
		node->QueueFree();
	}
}

bool ScenePanel::HandleTreeInput(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::Key:
		if (event.m_pressed && event.m_keyCode == GLFW_KEY_DELETE)
		{
			DeleteSelected();
			return true;
		}
		break;
	case InputEventType::MouseButton:
		if (event.m_pressed && event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_tree->Select(m_tree->GetItemAtPosition(event.m_position), true);
			m_context.m_editorUI->ShowMenu(m_tree, event.m_position + m_tree->GetGlobalPosition(), {
				{ "Add Child Node...", [this]() { OpenCreateNode(); } },
				{ "Rename", [this]() { m_tree->EditSelected(); } },
				{ "Delete", [this]() { DeleteSelected(); } }
			});
			return true;
		}
		break;
	}

	return false;
}

bool ScenePanel::CanReparent(ObjectID source, uint64_t target) const
{
	auto found = m_nodeItems.find(target);
	Node* child = ObjectDatabase::GetInstance<Node>(source);
	Node* parent = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(found->second) : nullptr;
	if (child == nullptr || parent == nullptr || child == m_context.m_sceneTree->GetScene()
		|| child->GetParent() == parent || child->IsQueuedForDeletion() || parent->IsQueuedForDeletion())
	{
		return false;
	}

	for (Node* current = parent; current != nullptr; current = current->GetParent())
	{
		if (current == child)
		{
			return false;
		}
	}

	return child->GetSceneTree() == m_context.m_sceneTree && parent->GetSceneTree() == m_context.m_sceneTree;
}

void ScenePanel::UpdateItems()
{
	std::string filter = m_filter->GetText().ToUtf8();
	std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	std::vector<Tree::Item> items;
	m_nodeItems.clear();
	std::function<bool(Node*, uint64_t)> append = [&](Node* node, uint64_t parent)
	{
		if (node == nullptr || node->IsQueuedForDeletion())
		{
			return false;
		}

		uint64_t const key = ((static_cast<uint64_t>(node->GetObjectID().GetUID()) << 32) | node->GetObjectID().GetIndex()) + 1;
		m_nodeItems[key] = node->GetObjectID();
		size_t const index = items.size();
		items.push_back({ key, parent, node->GetName(), true, true, GetThemeIcon(node->GetClassName(), "EditorIcons"), EditorIcons::GetIconColor(node->GetClassName(), this) });
		if (node->GetScriptInstance() != nullptr)
		{
			items[index].m_actions.push_back({ GetThemeIcon("CSharpScript", "EditorIcons"), GetThemeColor("tn", "EditorIcons"), false });
		}

		if (auto* canvas = dynamic_cast<CanvasItem*>(node))
		{
			items[index].m_actions.push_back({ GetThemeIcon(canvas->IsVisible() ? "Visible" : "Hidden", "EditorIcons"),
				GetThemeColor("tn", "EditorIcons"), true });
			items[index].m_dimmed = !canvas->IsVisibleInHierarchy();
		}
		else
		{
			items[index].m_actions.push_back({ GetThemeIcon("Visible", "EditorIcons"),
				GetThemeColor("font_color", "EditorStatusLabel"), false });
		}
		std::string lower = node->GetName().ToUtf8();
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		bool visible = filter.empty() || lower.find(filter) != std::string::npos;
		for (Node* child : node->GetChildren())
		{
			visible = append(child, key) || visible;
		}

		items[index].m_visible = visible;
		return visible;
	};
	append(m_context.m_sceneTree->GetScene(), 0);
	bool const any = std::any_of(items.begin(), items.end(), [](Tree::Item const& item) { return item.m_visible; });
	m_empty->SetText(filter.empty() ? "No scene open" : "No matching nodes");
	m_empty->SetVisible(!any);
	m_tree->SetItems(std::move(items), !filter.empty());
	uint64_t selected = 0;
	for (auto const& item : m_nodeItems)
	{
		if (item.second == m_context.m_selection->GetSelected())
		{
			selected = item.first;
			break;
		}
	}

	m_tree->Select(selected);
}
