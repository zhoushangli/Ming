#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/Popup/CreateNodePopup.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"

#include <algorithm>
#include <cctype>

namespace
{
bool CanCreateClass(ClassInfo const* info)
{
	return info != nullptr && info->m_apiType == ApiType::Runtime && !info->m_isVirtual && info->m_creator;
}
}

void CreateNodePopup::Open(ObjectID parentID)
{
	Reset();
	m_parentID      = parentID;
	m_openRequested = true;
}

void CreateNodePopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (m_popup == nullptr && !m_openRequested)
	{
		return;
	}

	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "Create Node", content);
		m_search = MemNew<LineEdit>();
		m_search->SetPlaceholder("Search node types");
		content->AddNode(m_search);
		m_search->SetTextChangedCallback([this](String const&) { RebuildTypes(); });
		auto* scroll = MemNew<ScrollContainer>();
		scroll->SetVerticalSizeFlags(Control::Size_ExpandFill);
		scroll->SetCustomMinimumSize(Vector2(0.f, static_cast<float>(context.m_rootControl->GetThemeConstant("popup_height", "Editor"))));
		content->AddNode(scroll);
		m_types = MemNew<Tree>();
		scroll->AddNode(m_types);
		m_types->SetSelectedCallback([this](uint64_t id)
		{
			auto found = m_typeNames.find(id);
			m_selectedClass = found != m_typeNames.end() ? found->second : "";
			if (m_createButton != nullptr)
			{
				m_createButton->SetDisabled(!CanCreateClass(ClassDatabase::GetClassInfo(m_selectedClass)));
			}
		});
		m_types->SetActivatedCallback([this](uint64_t)
		{
			if (CreateSelectedNode(m_context))
			{
				m_popup->Close();
			}
		});
		auto* buttons = MemNew<HBoxContainer>();
		content->AddNode(buttons);
		m_createButton = EditorPopupUtils::AddButton(buttons, "Create", [this]()
		{
			if (CreateSelectedNode(m_context))
			{
				m_popup->Close();
			}
		});
		EditorPopupUtils::AddButton(buttons, "Cancel", [this]() { m_popup->Close(); });
	}

	if (m_openRequested)
	{
		m_openRequested = false;
		m_search->SetText("");
		RebuildTypes();
		EditorPopupUtils::Open(m_popup, context.m_rootControl, Vector2(static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_width", "Editor")), static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_height", "Editor"))));
		m_search->GrabFocus();
	}

	m_createButton->SetDisabled(!CanCreateClass(ClassDatabase::GetClassInfo(m_selectedClass)));

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(m_popup, Vector2(static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_width", "Editor")), static_cast<float>(context.m_rootControl->GetThemeConstant("node_popup_height", "Editor"))));
	}
}

void CreateNodePopup::RebuildTypes()
{
	std::string filter = m_search->GetText().ToUtf8();
	std::transform(filter.begin(), filter.end(), filter.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
	auto classes = ClassDatabase::GetRegisteredClasses(true);
	m_typeNames.clear();
	std::map<std::string, uint64_t> keys;
	uint64_t next = 1;
	for (ClassInfo const* info : classes)
	{
		if (info != nullptr && info->m_apiType == ApiType::Runtime && ClassDatabase::IsSubclassOf(info->m_className, "Node"))
		{
			keys[info->m_className] = next++;
		}
	}

	std::vector<Tree::Item> items;
	for (ClassInfo const* info : classes)
	{
		if (info == nullptr || keys.count(info->m_className) == 0)
		{
			continue;
		}

		std::string lower = info->m_className;
		std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
		if (!filter.empty() && lower.find(filter) == std::string::npos)
		{
			continue;
		}

		uint64_t const key = keys[info->m_className];
		items.push_back({ key, filter.empty() && keys.count(info->m_parentClassName) != 0
			? keys[info->m_parentClassName] : 0, info->m_className, true, false,
			m_types->GetThemeIcon(info->m_className, "EditorIcons"), EditorIcons::GetIconColor(info->m_className, m_types) });
		m_typeNames[key] = info->m_className;
	}

	m_types->SetItems(std::move(items), !filter.empty());
	bool selectedExists = false;
	for (auto const& entry : m_typeNames)
	{
		selectedExists = selectedExists || entry.second == m_selectedClass;
	}

	if (!selectedExists)
	{
		m_selectedClass.clear();
	}
}

bool CreateNodePopup::CreateSelectedNode(EditorUIContext& context)
{
	if (m_selectedClass.empty())
	{
		return false;
	}

	ClassInfo const* selectedInfo = ClassDatabase::GetClassInfo(m_selectedClass);
	if (!CanCreateClass(selectedInfo))
	{
		return false;
	}

	Object* object = ClassDatabase::CreateInstance(m_selectedClass);
	Node*   node   = dynamic_cast<Node*>(object);
	if (node == nullptr)
	{
		MemDelete(object);
		return false;
	}

	if (context.m_sceneTree == nullptr)
	{
		MemDelete(node);
		return false;
	}

	Node* sceneRoot = context.m_sceneTree->GetScene();
	// If the scene is empty, we will make the new node the root of the scene.
	if (sceneRoot == nullptr)
	{
		context.m_sceneTree->ChangeScene(node);
	}
	else
	{
		Node* parent = ResolveCreateParent(context);
		// If the parent is invalid, we will add the new node to the scene root.
		if (sceneRoot != nullptr && parent == nullptr)
		{
			parent = sceneRoot;
		}

		node->SetName(m_selectedClass);

		parent->AddNode(node);
	}

	EditorNode::Get()->MarkSceneDirty();

	if (context.m_selection != nullptr)
	{
		context.m_selection->SetSelected(node->GetObjectID());
	}

	return true;
}

Node* CreateNodePopup::ResolveCreateParent(EditorUIContext const& context) const
{
	if (context.m_sceneTree == nullptr || !m_parentID.IsValid())
	{
		return nullptr;
	}

	// Resolve at creation time because the context node may disappear while the modal is open.
	Node* parent = ObjectDatabase::GetInstance<Node>(m_parentID);
	if (parent == nullptr || parent->GetSceneTree() != context.m_sceneTree)
	{
		return nullptr;
	}

	if (!parent->GetSerializable())
	{
		EditorNode::Get()->m_editorUI->Warning("Cannot Create Node", "Cannot add a child to a non-serializable node.");
		return nullptr;
	}

	return parent;
}

void CreateNodePopup::Reset()
{
	m_selectedClass.clear();
	m_parentID      = ObjectID::Invalid;
	m_openRequested = false;
}
