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
#include "MingEngine/Scene/3D/Node3D.hpp"

#include <algorithm>
#include <cctype>

ScenePanel::ScenePanel() : EditorPanel("Scene")
{
}

void ScenePanel::OnInitialize()
{
	m_createNodePopup.SetCreateCallback(
		[this](std::string const& type, std::string& error)
		{
			return CreateNode(type, error);
		});
	auto* layout = MemNew<VBoxContainer>();
	layout->AddThemeChangedCallback(
		[control = layout]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	auto* margin = MemNew<MarginContainer>();
	margin->AddThemeChangedCallback(
		[control = margin]()
		{
			control->SetMargins(
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")),
				static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")));
		});
	AddNode(margin);
	margin->AddNode(layout);
	layout->UpdateSize(GetSize());
	m_filter = MemNew<LineEdit>();
	m_filter->SetPlaceholder("Filter nodes");
	m_filter->SetIcon(GetThemeIcon("Search", "EditorIcons"));
	m_filter->SetMinimumCharacterWidth(1);
	auto* tools = MemNew<HBoxContainer>();
	tools->AddThemeChangedCallback(
		[control = tools]()
		{
			control->SetSeparation(static_cast<float>(control->GetThemeConstant("panel_margin", "Editor")) * 0.5f);
		});
	layout->AddNode(tools);
	Button* add = EditorPopupUtils::AddButton(
		tools,
		"",
		[this]()
		{
			OpenCreateNode();
		});
	add->SetIcon(GetThemeIcon("Add", "EditorIcons"));
	add->SetThemeTypeVariation("EditorToolbarButton");
	m_filter->SetHorizontalSizeFlags(Size_ExpandFill);
	tools->AddNode(m_filter);
	m_empty       = EditorPopupUtils::AddLabel(layout, "Create a root node", "EditorStatusLabel");
	m_rootActions = MemNew<VBoxContainer>();
	layout->AddNode(m_rootActions);
	for (char const* type : { "Node3D", "Control" })
	{
		EditorPopupUtils::AddButton(
			m_rootActions,
			type,
			[this, type]()
			{
				m_createSceneID  = ObjectID::Invalid;
				m_createParentID = ObjectID::Invalid;
				std::string error;
				if (!CreateNode(type, error))
				{
					m_context.m_editorUI->Warning("Create Root", error);
				}
			});
	}
	EditorPopupUtils::AddButton(
		m_rootActions,
		"Other Node...",
		[this]()
		{
			OpenCreateNode();
		});
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);
	m_tree = MemNew<Tree>();
	m_tree->SetClipText(true);
	scroll->AddNode(m_tree);
	m_tree->SetItemActionCallback(
		[this](uint64_t key, int action)
		{
			auto  found = m_nodeItems.find(key);
			auto* item  = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(found->second) : nullptr;
			PropertyInfo const* visibility = item != nullptr ? ClassDatabase::FindProperty(item->GetClassName(), "visible") : nullptr;
			if (item != nullptr && !item->IsQueuedForDeletion() && visibility != nullptr
				&& action == (item->GetScriptInstance() != nullptr ? 1 : 0))
			{
				bool const before = visibility->GetGetter()->Invoke(item, {}).As<bool>();
				visibility->GetSetter()->Invoke(item, { !before });
				auto* editor = EditorNode::Get();
				auto& undo   = editor->GetUndoRedo();
				if (undo.CreateAction("Toggle Visibility", m_context.m_sceneTree->GetScene()->GetObjectID()))
				{
					undo.AddDoProperty(item, "visible", !before);
					undo.AddUndoProperty(item, "visible", before);
					undo.AddDoMethod(editor, "MarkSceneDirty");
					undo.AddUndoMethod(editor, "MarkSceneDirty");
					undo.CommitAction(false);
				}
				EditorNode::Get()->MarkSceneDirty();
			}
		});
	m_tree->SetSelectedCallback(
		[this](uint64_t key)
		{
			auto found = m_nodeItems.find(key);
			m_context.m_selection->SetSelected(found != m_nodeItems.end() ? found->second : ObjectID::Invalid);
		});
	m_tree->SetRenamedCallback(
		[this](uint64_t key, String const& name)
		{
			auto  found = m_nodeItems.find(key);
			Node* node  = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(found->second) : nullptr;
			if (node != nullptr && !node->IsQueuedForDeletion() && node->GetName() != name)
			{
				String const before = node->GetName();
				node->SetName(name);
				auto* editor = EditorNode::Get();
				auto& undo   = editor->GetUndoRedo();
				if (undo.CreateAction("Rename Node", m_context.m_sceneTree->GetScene()->GetObjectID()))
				{
					undo.AddDoMethod(node, "SetName", { node->GetName() });
					undo.AddUndoMethod(node, "SetName", { before });
					undo.AddDoMethod(editor, "MarkSceneDirty");
					undo.AddUndoMethod(editor, "MarkSceneDirty");
					undo.CommitAction(false);
				}
				editor->MarkSceneDirty();
			}
		});
	m_tree->SetInputCallback(
		[this](InputEvent const& event)
		{
			return HandleTreeInput(event);
		});
	m_tree->SetDragCallbacks(
		[this](Vector2 const& position, GuiDragData& data)
		{
			auto  found = m_nodeItems.find(m_tree->GetItemAtPosition(position));
			Node* node  = found != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(found->second) : nullptr;
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
				Node*      child  = ObjectDatabase::GetInstance<Node>(data.m_objectID);
				auto*      editor = EditorNode::Get();
				auto&      undo   = editor->GetUndoRedo();
				bool const recording =
					undo.CreateAction("Reparent Node", m_context.m_sceneTree->GetScene()->GetObjectID());
				auto snapshot = [&](bool backwards)
				{
					Node*     parent = child->GetParent();
					int const index  = static_cast<int>(
						std::find(parent->GetChildren().begin(), parent->GetChildren().end(), child)
						- parent->GetChildren().begin());
					auto add = [&](Object* target, char const* method, std::vector<Variant> arguments)
					{
						if (backwards)
						{
							undo.AddUndoMethod(target, method, arguments);
						}
						else
						{
							undo.AddDoMethod(target, method, arguments);
						}
					};
					add(child, "Reparent", { parent, false });
					add(parent, "MoveChild", { child, index });
					add(child, "SetName", { child->GetName() });
					if (auto* spatial = dynamic_cast<Node3D*>(child))
					{
						add(child, "SetLocalPosition", { spatial->GetLocalPosition() });
						add(child, "SetLocalOrientation", { spatial->GetLocalOrientation() });
						add(child, "SetLocalScale", { spatial->GetLocalScale() });
					}
					add(editor, "MarkSceneDirty", {});
				};
				if (recording)
				{
					snapshot(true);
				}
				child->Reparent(ObjectDatabase::GetInstance<Node>(m_nodeItems.at(key)), true);
				if (recording)
				{
					snapshot(false);
					undo.CommitAction(false);
				}
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
	Node* scene      = m_context.m_sceneTree->GetScene();
	m_createSceneID  = scene != nullptr ? scene->GetObjectID() : ObjectID::Invalid;
	m_createParentID = m_context.m_selection->GetSelected();
	if (!m_createParentID.IsValid())
	{
		m_createParentID = m_createSceneID;
	}

	m_createNodePopup.Open(m_createParentID);
}

bool ScenePanel::CreateNode(std::string const& type, std::string& error)
{
	Node*          scene   = m_context.m_sceneTree->GetScene();
	ObjectID const sceneID = scene != nullptr ? scene->GetObjectID() : ObjectID::Invalid;
	Node*          parent  = ObjectDatabase::GetInstance<Node>(m_createParentID);
	if (sceneID != m_createSceneID
		|| (scene != nullptr
			&& (parent == nullptr || parent->IsQueuedForDeletion() || parent->GetSceneTree() != m_context.m_sceneTree
				|| !parent->GetSerializable())))
	{
		error = "The creation target is no longer editable. Close and reopen this dialog.";
		return false;
	}

	if (scene != nullptr)
	{
		Node* ancestor = parent;
		while (ancestor != nullptr && ancestor != scene)
		{
			ancestor = ancestor->GetParent();
		}

		if (ancestor != scene)
		{
			error = "The creation target is outside the current scene.";
			return false;
		}
	}

	Object* object = ClassDatabase::CreateInstance(type);
	Node*   node   = dynamic_cast<Node*>(object);
	if (node == nullptr)
	{
		MemDelete(object);
		error = "Could not create the selected node type.";
		return false;
	}

	node->SetName(type);
	if (scene == nullptr)
	{
		m_context.m_sceneTree->ChangeScene(node);
		m_context.m_sceneTree->FlushPendingNode();
		EditorNode::Get()->NewRootCreated();
	}
	else
	{
		parent->AddNode(node);
		EditorNode::Get()->GetUndoRedo().ClearHistory(scene->GetObjectID());
	}

	m_context.m_selection->SetSelected(node->GetObjectID());
	EditorNode::Get()->MarkSceneDirty();
	return true;
}

void ScenePanel::DeleteSelected()
{
	Node* node = ObjectDatabase::GetInstance<Node>(m_context.m_selection->GetSelected());
	if (node != nullptr && node->GetSceneTree() == m_context.m_sceneTree && !node->IsQueuedForDeletion())
	{
		if (node == m_context.m_sceneTree->GetScene())
		{
			EditorNode::Get()->RequestCloseScene();
			return;
		}

		EditorNode::Get()->GetUndoRedo().ClearHistory(m_context.m_sceneTree->GetScene()->GetObjectID());
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
			m_context.m_editorUI->ShowMenu(
				m_tree,
				event.m_position + m_tree->GetGlobalPosition(),
				{ { "Add Child Node...",
					[this]()
					{
						OpenCreateNode();
					} },
				  { "Rename",
					[this]()
					{
						m_tree->EditSelected();
					} },
				  { "Delete",
					[this]()
					{
						DeleteSelected();
					} } });
			return true;
		}
		break;
	}

	return false;
}

bool ScenePanel::CanReparent(ObjectID source, uint64_t target) const
{
	auto  found  = m_nodeItems.find(target);
	Node* child  = ObjectDatabase::GetInstance<Node>(source);
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
	std::transform(
		filter.begin(),
		filter.end(),
		filter.begin(),
		[](unsigned char ch)
		{
			return static_cast<char>(std::tolower(ch));
		});
	std::vector<Tree::Item> items;
	m_nodeItems.clear();
	std::function<bool(Node*, uint64_t)> append = [&](Node* node, uint64_t parent)
	{
		if (node == nullptr || node->IsQueuedForDeletion())
		{
			return false;
		}

		uint64_t const key =
			((static_cast<uint64_t>(node->GetObjectID().GetUID()) << 32) | node->GetObjectID().GetIndex()) + 1;
		m_nodeItems[key]   = node->GetObjectID();
		size_t const index = items.size();
		items.push_back(
			{ key,
			  parent,
			  node->GetName(),
			  true,
			  true,
			  GetThemeIcon(node->GetClassName(), "EditorIcons"),
			  EditorIcons::GetIconColor(node->GetClassName(), this) });
		if (node->GetScriptInstance() != nullptr)
		{
			items[index].m_actions.push_back(
				{ GetThemeIcon("CSharpScript", "EditorIcons"), GetThemeColor("tn", "EditorIcons"), false });
		}

		if (auto* canvas = dynamic_cast<CanvasItem*>(node))
		{
			items[index].m_dimmed = !canvas->IsVisibleInHierarchy();
			items[index].m_actions.push_back(
				{ GetThemeIcon(canvas->IsVisible() ? "Visible" : "Hidden", "EditorIcons"),
				  GetThemeColor(items[index].m_dimmed ? "font_color" : "tn", items[index].m_dimmed ? "EditorStatusLabel" : "EditorIcons"),
				  true });
		}
		else if (auto* spatial = dynamic_cast<Node3D*>(node))
		{
			items[index].m_dimmed = !spatial->IsVisibleInHierarchy();
			items[index].m_actions.push_back(
				{ GetThemeIcon(spatial->IsVisible() ? "Visible" : "Hidden", "EditorIcons"),
				  GetThemeColor(items[index].m_dimmed ? "font_color" : "tn", items[index].m_dimmed ? "EditorStatusLabel" : "EditorIcons"),
				  true });
		}
		std::string lower = node->GetName().ToUtf8();
		std::transform(
			lower.begin(),
			lower.end(),
			lower.begin(),
			[](unsigned char ch)
			{
				return static_cast<char>(std::tolower(ch));
			});
		bool visible = filter.empty() || lower.find(filter) != std::string::npos;
		for (Node* child : node->GetChildren())
		{
			visible = append(child, key) || visible;
		}

		items[index].m_visible = visible;
		return visible;
	};
	append(m_context.m_sceneTree->GetScene(), 0);
	bool const any = std::any_of(
		items.begin(),
		items.end(),
		[](Tree::Item const& item)
		{
			return item.m_visible;
		});
	m_rootActions->SetVisible(m_context.m_sceneTree->GetScene() == nullptr);
	m_empty->SetText(filter.empty() ? "Create a root node" : "No matching nodes");
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
