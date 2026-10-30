#include "MingEngine/Editor/UI/EditorUI.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/SplitContainer.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{
uint64_t ItemKey(ObjectID id)
{
	return (static_cast<uint64_t>(id.GetUID()) << 32 | id.GetIndex()) + 1;
}

std::string Lower(std::string text)
{
	std::transform(
		text.begin(),
		text.end(),
		text.begin(),
		[](unsigned char c)
		{
			return static_cast<char>(std::tolower(c));
		});
	return text;
}

void Place(Control* control, float x, float y, float width, float height)
{
	control->SetPosition(Vector2(x, y));
	control->UpdateSize(Vector2(std::max(0.f, width), std::max(0.f, height)));
}

void FinishEditing(Node* node)
{
	if (auto* edit = dynamic_cast<LineEdit*>(node))
	{
		edit->EndEditing(LineEdit::EditEndReason::FocusLost);
	}

	for (Node* child : node->GetChildren())
	{
		FinishEditing(child);
	}
}
}

void EditorUI::Render(EditorUIContext& context)
{
	if (g_engine->m_imguiSystem == nullptr)
	{
		RenderNative(context);
	}
	else
	{
		RenderLegacy(context);
	}
}

void EditorUI::InitializeNative(EditorNode* owner)
{
	if (m_nativeRoot != nullptr)
	{
		return;
	}

	m_owner      = owner;
	m_nativeFont = JetBrainsMonoFont::CreateNativeFont();
	m_nativeRoot = MemNew<ColorRect>();
	m_nativeRoot->SetName("EditorUI");
	m_nativeRoot->SetSerializable(false);
	m_nativeRoot->SetColor(Color(25, 29, 37));
	m_nativeRoot->SetMouseFilter(Control::MouseFilter::Stop);
	m_nativeRoot->SetClipContents(true);
	owner->AddNode(m_nativeRoot);
	auto* toolbar = MemNew<HBoxContainer>();
	toolbar->SetSeparation(10.f);
	m_nativeRoot->AddNode(toolbar);
	m_toolbar  = toolbar;
	auto* open = MemNew<Button>();
	open->SetText("Open Scene");
	open->SetPressedCallback(
		[this]()
		{
			ShowNativePopup(2, "Open scene using a res:// path");
		});
	toolbar->AddNode(open);
	auto* save = MemNew<Button>();
	save->SetText("Save");
	save->SetPressedCallback(
		[this]()
		{
			FinishNativeEditing();
			if (!m_owner->SaveScene())
			{
				Warning("Save failed", "Open an existing scene before saving.");
			}
		});
	toolbar->AddNode(save);
	m_sceneTitle = MemNew<Label>();
	toolbar->AddNode(m_sceneTitle);
	m_tabs = MemNew<TabContainer>();
	m_nativeRoot->AddNode(m_tabs);
	m_scenePage = MemNew<Control>();
	m_scenePage->SetClipContents(true);
	m_filePage = MemNew<Control>();
	m_filePage->SetClipContents(true);
	m_tabs->AddPage("Scene", m_scenePage);
	m_tabs->AddPage("FileSystem", m_filePage);
	m_sceneFilter = MemNew<LineEdit>();
	m_sceneFilter->SetPlaceholder("Filter nodes");
	m_scenePage->AddNode(m_sceneFilter);
	m_sceneScroll = MemNew<ScrollContainer>();
	m_scenePage->AddNode(m_sceneScroll);
	m_nativeSceneTree = MemNew<Tree>();
	m_nativeSceneTree->SetMouseFilter(Control::MouseFilter::Pass);
	m_sceneScroll->AddNode(m_nativeSceneTree);
	m_nativeSceneTree->SetSelectedCallback(
		[this](uint64_t key)
		{
			auto it = m_nodeItems.find(key);
			m_owner->GetSelection().SetSelected(it != m_nodeItems.end() ? it->second : ObjectID::Invalid);
		});
	m_nativeSceneTree->SetRenamedCallback(
		[this](uint64_t key, String const& name)
		{
			auto  it   = m_nodeItems.find(key);
			Node* node = it != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(it->second) : nullptr;
			if (node != nullptr && !node->IsQueuedForDeletion() && node->GetName() != name)
			{
				node->SetName(name);
				m_owner->MarkSceneDirty();
			}
		});
	m_fileFilter = MemNew<LineEdit>();
	m_fileFilter->SetPlaceholder("Filter files");
	m_fileFilter->SetTextChangedCallback(
		[this](String const&)
		{
			m_filesDirty = true;
		});
	m_filePage->AddNode(m_fileFilter);
	auto* refresh = MemNew<Button>();
	refresh->SetText("Refresh");
	refresh->SetPressedCallback(
		[this]()
		{
			g_engine->m_fileSystem->ScanResourceTree();
			m_filesDirty = true;
		});
	m_filePage->AddNode(refresh);
	m_fileScroll = MemNew<ScrollContainer>();
	m_filePage->AddNode(m_fileScroll);
	m_nativeFileTree = MemNew<Tree>();
	m_nativeFileTree->SetMouseFilter(Control::MouseFilter::Pass);
	m_fileScroll->AddNode(m_nativeFileTree);
	m_nativeFileTree->SetActivatedCallback(
		[this](uint64_t key)
		{
			auto it = m_fileItems.find(key);
			if (it != m_fileItems.end() && it->second.HasExtension(".tscn"))
			{
				FinishNativeEditing();
				m_owner->RequestLoadScene(it->second);
			}
		});
	m_sceneView = MemNew<SubViewportContainer>();
	m_nativeRoot->AddNode(m_sceneView);
	m_sceneView->SetInputCallback(
		[this](InputEvent const& event)
		{
			if (event.m_type == InputEventType::WindowFocus && !event.m_focused)
			{
				m_navigating = false;
			}

			if (event.m_type == InputEventType::MouseButton)
			{
				if (event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
				{
					m_navigating = event.m_pressed && m_sceneView->IsPointInside(event.m_position);
					if (m_navigating)
					{
						m_sceneView->GrabFocus();
					}
				}
				else if (event.m_mouseButton == GLFW_MOUSE_BUTTON_LEFT && event.m_pressed)
				{
					m_owner->OnMouseDown(
						ToKeyCode(KeyCode::LeftMouse),
						event.m_position + m_sceneView->GetGlobalPosition());
				}
			}

			return event.m_type != InputEventType::Key;
		});
	m_inspectorBackground = MemNew<ColorRect>();
	m_inspectorBackground->SetColor(Color(32, 38, 48));
	m_inspectorBackground->SetClipContents(true);
	m_nativeRoot->AddNode(m_inspectorBackground);
	m_inspectorPanel.GetHost().Initialize(owner->GetSceneTree());
	m_inspectorPanel.GetHost().SetFont(m_nativeFont);
	m_inspectorPanel.GetHost().GetRootControl()->Reparent(m_inspectorBackground, false);
	m_inspectorTitle = MemNew<Label>();
	m_inspectorTitle->SetText("Inspector");
	m_nativeRoot->AddNode(m_inspectorTitle);
	m_emptyInspector = MemNew<Label>();
	m_emptyInspector->SetText("No node selected");
	m_nativeRoot->AddNode(m_emptyInspector);
	m_nativePopup = MemNew<PopupPanel>();
	m_nativePopup->SetModal(true);
	m_nativeRoot->AddNode(m_nativePopup);
	m_popupPanel = MemNew<ColorRect>();
	m_popupPanel->SetColor(Color(40, 48, 61));
	m_popupPanel->SetMouseFilter(Control::MouseFilter::Stop);
	m_popupPanel->SetClipContents(true);
	m_nativePopup->AddNode(m_popupPanel);
	m_popupMessage = MemNew<Label>();
	m_popupPanel->AddNode(m_popupMessage);
	m_popupPath = MemNew<LineEdit>();
	m_popupPanel->AddNode(m_popupPath);
	for (int i = 0; i < 3; ++i)
	{
		m_popupButtons[i] = MemNew<Button>();
		m_popupButtons[i]->SetPressedCallback(
			[this, i]()
			{
				AcceptNativePopup(i);
			});
		m_popupPanel->AddNode(m_popupButtons[i]);
	}

	m_nativePopup->SetKeyCallback(
		[this](InputEvent const& event)
		{
			if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_ESCAPE)
			{
				AcceptNativePopup(1);
			}
			else if (m_popupMode == 2 && m_popupPath->HasFocus())
			{
				if (event.m_type == InputEventType::Key && event.m_pressed && event.m_keyCode == GLFW_KEY_ENTER)
				{
					AcceptNativePopup(0);
				}
				else
				{
					m_popupPath->GuiInput(event);
				}
			}
			else if (
				Control* focus = m_nativePopup->GetViewport()->GetGuiFocus();
				focus != nullptr && focus != m_nativePopup)
			{
				focus->GuiInput(event);
			}
		});
	JetBrainsMonoFont::ApplyNativeFont(m_nativeRoot, m_nativeFont);
	owner->GetSceneTree()->SetSceneViewport(m_sceneView->GetSubViewport());
	g_engine->m_fileSystem->ScanResourceTree();
	InitializeNativeDragDrop();
	InitializeNativeDocking();
}

void EditorUI::SetNativeDragPreview(Control* source, String const& text)
{
	auto* background = MemNew<ColorRect>();
	background->SetColor(Color(45, 58, 76));
	auto* label = MemNew<Label>();
	label->SetFont(m_nativeFont);
	label->SetText(text);
	label->UpdateSize(label->GetCombinedMinimumSize());
	background->AddNode(label);
	label->SetPosition(Vector2(8.f, 6.f));
	background->UpdateSize(label->GetCombinedMinimumSize() + Vector2(16.f, 12.f));
	source->SetDragPreview(background);
}

bool EditorUI::CanReparentNativeNode(ObjectID source, uint64_t target) const
{
	auto const it = m_nodeItems.find(target);
	Node* child = ObjectDatabase::GetInstance<Node>(source);
	Node* parent = it != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(it->second) : nullptr;
	Node* scene = m_owner->GetSceneTree()->GetScene();
	auto inScene = [scene](Node* node)
	{
		for (; node != nullptr; node = node->GetParent())
		{
			if (node->IsQueuedForDeletion())
			{
				return false;
			}

			if (node == scene)
			{
				return true;
			}
		}

		return false;
	};
	if (child == nullptr || parent == nullptr || child == scene || child->GetParent() == parent
		|| !inScene(child) || !inScene(parent))
	{
		return false;
	}

	for (Node* node = parent; node != nullptr; node = node->GetParent())
	{
		if (node == child)
		{
			return false;
		}
	}

	return true;
}

bool EditorUI::CanMoveNativeFile(VirtualPath const& source, uint64_t target) const
{
	auto const it = m_fileItems.find(target);
	if (it == m_fileItems.end() || !source.IsValid() || source.IsRoot() || source == it->second
		|| source.GetParent() == it->second)
	{
		return false;
	}

	FileSystem* files = g_engine->m_fileSystem;
	std::filesystem::path sourcePath;
	std::filesystem::path targetPath;
	std::error_code error;
	if (!files->TryGetPhysicalPath(source, sourcePath) || !files->TryGetPhysicalPath(it->second, targetPath)
		|| !std::filesystem::exists(sourcePath, error) || error
		|| !std::filesystem::is_directory(targetPath, error) || error)
	{
		return false;
	}

	std::string const prefix = source.GetString() + "/";
	if (it->second.GetString().compare(0, prefix.size(), prefix) == 0)
	{
		return false;
	}

	return !std::filesystem::exists(targetPath / sourcePath.filename(), error) && !error;
}

void EditorUI::InitializeNativeDragDrop()
{
	m_nativeSceneTree->SetDragCallbacks(
		[this](Vector2 const& position, GuiDragData& data)
		{
			auto const it = m_nodeItems.find(m_nativeSceneTree->GetItemAtPosition(position));
			Node* node = it != m_nodeItems.end() ? ObjectDatabase::GetInstance<Node>(it->second) : nullptr;
			if (node == nullptr || node->IsQueuedForDeletion() || node == m_owner->GetSceneTree()->GetScene())
			{
				return false;
			}

			data = { "node", node->GetObjectID(), node->GetName() };
			SetNativeDragPreview(m_nativeSceneTree, node->GetName());
			return true;
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			return data.m_type == String("node")
				&& CanReparentNativeNode(data.m_objectID, m_nativeSceneTree->GetItemAtPosition(position));
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			uint64_t const target = m_nativeSceneTree->GetItemAtPosition(position);
			if (!CanReparentNativeNode(data.m_objectID, target))
			{
				return;
			}

			Node* child = ObjectDatabase::GetInstance<Node>(data.m_objectID);
			Node* parent = ObjectDatabase::GetInstance<Node>(m_nodeItems.at(target));
			child->Reparent(parent, true);
			m_owner->GetSelection().SetSelected(data.m_objectID);
			m_owner->MarkSceneDirty();
			UpdateSceneItems();
		});
	m_nativeFileTree->SetDragCallbacks(
		[this](Vector2 const& position, GuiDragData& data)
		{
			auto const it = m_fileItems.find(m_nativeFileTree->GetItemAtPosition(position));
			if (it == m_fileItems.end() || it->second.IsRoot())
			{
				return false;
			}

			data = { "file", ObjectID::Invalid, it->second.GetString() };
			SetNativeDragPreview(m_nativeFileTree, data.m_value);
			return true;
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			VirtualPath source;
			return data.m_type == String("file") && VirtualPath::TryParse(data.m_value.ToUtf8(), source)
				&& CanMoveNativeFile(source, m_nativeFileTree->GetItemAtPosition(position));
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			VirtualPath source;
			uint64_t const target = m_nativeFileTree->GetItemAtPosition(position);
			if (!VirtualPath::TryParse(data.m_value.ToUtf8(), source) || !CanMoveNativeFile(source, target))
			{
				return;
			}

			VirtualPath moved;
			std::string error;
			bool const success = g_engine->m_fileSystem->Move(source, m_fileItems.at(target), moved, error);
			g_engine->m_fileSystem->ScanResourceTree();
			m_filesDirty = true;
			UpdateFileItems();
			if (success)
			{
				auto const it = m_fileKeys.find(moved.GetString());
				m_nativeFileTree->Select(it != m_fileKeys.end() ? it->second : 0);
			}
			else
			{
				Warning("Move failed", error);
			}
		});
}

void EditorUI::UpdateSceneItems()
{
	std::vector<Tree::Item> items;
	m_nodeItems.clear();
	std::string const                    filter = Lower(m_sceneFilter->GetText().ToUtf8());
	std::function<bool(Node*, uint64_t)> append = [&](Node* node, uint64_t parent)
	{
		if (node->IsQueuedForDeletion())
		{
			return false;
		}

		uint64_t const key   = ItemKey(node->GetObjectID());
		size_t const   index = items.size();
		items.push_back({ key, parent, node->GetName(), false, true });
		m_nodeItems[key] = node->GetObjectID();
		bool visible     = filter.empty() || Lower(node->GetName().ToUtf8()).find(filter) != std::string::npos;
		for (Node* child : node->GetChildren())
		{
			visible = append(child, key) || visible;
		}

		items[index].m_visible = visible;
		return visible;
	};

	if (Node* scene = m_owner->GetSceneTree()->GetScene())
	{
		append(scene, 0);
	}

	m_nativeSceneTree->SetItems(std::move(items), !filter.empty());
	ObjectID const selection = m_owner->GetSelection().GetSelected();
	m_nativeSceneTree->Select(selection.IsValid() ? ItemKey(selection) : 0);
}

void EditorUI::UpdateFileItems()
{
	if (!m_filesDirty)
	{
		return;
	}

	m_filesDirty = false;
	m_fileItems.clear();
	std::vector<Tree::Item>                         items;
	std::string const                               filter = Lower(m_fileFilter->GetText().ToUtf8());
	std::function<bool(FileEntry const*, uint64_t)> append = [&](FileEntry const* entry, uint64_t parent)
	{
		std::string const path   = entry->GetVirtualPath().GetString();
		auto              result = m_fileKeys.emplace(path, m_nextFileKey);
		if (result.second)
		{
			++m_nextFileKey;
		}

		uint64_t const key = result.first->second;
		m_fileItems[key]   = entry->GetVirtualPath();
		size_t const index = items.size();
		items.push_back({ key, parent, entry->GetName(), false, false });
		bool visible = filter.empty() || Lower(entry->GetName()).find(filter) != std::string::npos;
		for (auto const& child : entry->GetChildren())
		{
			visible = append(child.get(), key) || visible;
		}

		items[index].m_visible = visible;
		return visible;
	};

	if (FileEntry const* root = g_engine->m_fileSystem->GetResourceRootEntry())
	{
		append(root, 0);
	}

	m_nativeFileTree->SetItems(std::move(items), !filter.empty());
}

void EditorUI::RenderNative(EditorUIContext& context)
{
	InitializeNative(EditorNode::Get());
	context.m_editorUI       = this;
	Vector2 const size       = Vector2(static_cast<Viewport*>(context.m_sceneTree->GetRoot())->GetOutputResolution());
	float const   bodyHeight = std::max(1.f, size.y - 64.f);
	Place(m_nativeRoot, 0.f, 0.f, size.x, size.y);
	Place(m_toolbar, 12.f, 8.f, std::max(1.f, size.x - 24.f), 40.f);
	Place(m_dockSplits[0], 10.f, 58.f, std::max(1.f, size.x - 20.f), bodyHeight);
	UpdateNativeDocking();
	context.m_sceneTree->FlushContainerUpdate();
	for (Control* page : { m_scenePage, m_filePage })
	{
		float const width  = page->GetSize().x;
		float const height = page->GetSize().y;
		if (page == m_scenePage)
		{
			Place(m_sceneFilter, 0.f, 0.f, width, 34.f);
			Place(m_sceneScroll, 0.f, 42.f, width, height - 42.f);
		}
		else
		{
			Place(m_fileFilter, 0.f, 0.f, width, 34.f);
			Place(static_cast<Control*>(page->GetChildren()[1]), 0.f, 40.f, width, 34.f);
			Place(m_fileScroll, 0.f, 82.f, width, height - 82.f);
		}
	}

	Vector2 const inspectorSize = m_inspectorBackground->GetSize();
	Place(m_inspectorTitle, 12.f, 8.f, inspectorSize.x - 24.f, 24.f);
	Place(m_emptyInspector, 12.f, 42.f, inspectorSize.x - 24.f, 24.f);
	m_emptyInspector->SetVisible(!m_owner->GetSelection().GetSelected().IsValid());
	m_sceneTitle->SetText(m_owner->GetCurrentSceneName() + (m_owner->IsSceneDirty() ? " *" : ""));
	SetViewportRect(m_sceneView->GetGlobalPosition(), m_sceneView->GetSize());
	context.m_isViewportImageHovered = IsSceneHovered();
	UpdateSceneItems();
	UpdateFileItems();
	m_inspectorPanel.GetHost().BeginFrame();
	m_inspectorPanel.RenderNative(
		context,
		m_inspectorBackground->GetGlobalPosition() + Vector2(12.f, 46.f),
		Vector2(std::max(1.f, inspectorSize.x - 24.f), std::max(1.f, inspectorSize.y - 48.f)));
	m_inspectorPanel.GetHost().EndFrame();
	JetBrainsMonoFont::ApplyNativeFont(m_nativeRoot, m_nativeFont);
	context.m_sceneTree->FlushContainerUpdate();
	UpdateNativeDocking();
	Viewport* root = static_cast<Viewport*>(context.m_sceneTree->GetRoot());
	root->UpdateGuiDrag(root->GetGuiDragPosition());
	if (m_nativePopup->IsVisible())
	{
		m_nativePopup->UpdateSize(size);
		Place(
			m_popupPanel,
			std::max(0.f, (size.x - 600.f) * 0.5f),
			std::max(0.f, (size.y - 220.f) * 0.5f),
			600.f,
			220.f);
	}
}

bool EditorUI::IsSceneHovered() const
{
	if (m_sceneView == nullptr || m_sceneView->GetViewport()->GetGuiPopup() != nullptr
		|| m_sceneView->GetViewport()->IsGuiDragging())
	{
		return false;
	}

	return m_sceneView->IsPointInside(
		Vector2(g_engine->m_inputSystem->GetCursorClientPosition()) - m_sceneView->GetGlobalPosition());
}

bool EditorUI::IsSceneNavigationActive() const
{
	return m_navigating && m_sceneView != nullptr && m_sceneView->GetViewport()->IsWindowFocused()
		   && !m_sceneView->GetViewport()->IsGuiDragging()
		   && m_sceneView->HasFocus() && m_sceneView->GetViewport()->GetGuiPopup() == nullptr;
}

void EditorUI::FinishNativeEditing()
{
	if (m_nativeRoot != nullptr)
	{
		FinishEditing(m_nativeRoot);
		if (Control* root = m_inspectorPanel.GetHost().GetRootControl())
		{
			FinishEditing(root);
		}

		m_nativeRoot->GetViewport()->SetGuiFocus(nullptr);
	}
}

void EditorUI::ShowNativePopup(int mode, String const& message)
{
	if (m_nativePopup == nullptr)
	{
		return;
	}

	FinishNativeEditing();
	m_navigating = false;
	m_popupMode  = mode;
	m_popupMessage->SetText(message);
	Place(m_popupMessage, 20.f, 24.f, 560.f, 30.f);
	m_popupPath->SetVisible(mode == 2);
	m_popupPath->SetText("res://");
	Place(m_popupPath, 20.f, 78.f, 560.f, 36.f);
	m_popupButtons[0]->SetText(mode == 3 ? "Save & Continue" : mode == 2 ? "Open" : "OK");
	m_popupButtons[1]->SetText("Cancel");
	m_popupButtons[2]->SetText("Don't Save");
	for (int i = 0; i < 3; ++i)
	{
		m_popupButtons[i]->SetVisible(i == 0 || (i == 1 && mode != 1) || (i == 2 && mode == 3));
		Place(m_popupButtons[i], 20.f + i * 190.f, 156.f, 180.f, 40.f);
	}

	m_nativePopup->Open(m_nativeRoot);
	if (mode == 2)
	{
		m_popupPath->GrabFocus();
		m_popupPath->SetCursorColumn(6);
	}
}

void EditorUI::AcceptNativePopup(int choice)
{
	int const         mode = m_popupMode;
	VirtualPath const path(m_popupPath->GetText().ToUtf8());
	m_nativePopup->Close();
	m_popupMode = 0;
	if (mode == 3)
	{
		m_owner->ResolvePendingSceneAction(choice);
	}
	else if (mode == 2 && choice == 0)
	{
		m_owner->RequestLoadScene(path);
	}
}

void EditorUI::ShowUnsavedScene()
{
	ShowNativePopup(3, "This scene has unsaved changes. Save before continuing?");
}
