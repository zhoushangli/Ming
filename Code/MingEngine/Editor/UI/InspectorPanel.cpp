#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include <algorithm>
#include <cctype>

InspectorPanel::InspectorPanel() : EditorPanel("Inspector")
{
}

InspectorPanel::~InspectorPanel()
{
	ClearProperties();
}

void InspectorPanel::ClearProperties()
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this);
	}
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			delete entry.m_property;
		}
	}

	m_propertyGroups.clear();
	EditorPopupUtils::ClearContent(m_content);
}

void InspectorPanel::BuildInheritanceChain(std::string const& className)
{
	m_inheritanceChain.clear();
	std::string current = className;
	while (true)
	{
		m_inheritanceChain.push_back(current);
		ClassInfo const* info = ClassDatabase::GetClassInfo(current);
		if (current == "Node" || info == nullptr || info->m_parentClassName.empty()
			|| info->m_parentClassName == current)
		{
			break;
		}

		current = info->m_parentClassName;
	}
}

void InspectorPanel::RefreshValues()
{
	Node* node = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (node == nullptr || node->IsQueuedForDeletion() || node->GetSceneTree() != m_sceneTree)
	{
		return;
	}

	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			if (entry.m_getter != nullptr)
			{
				entry.m_property->UpdateValue(entry.m_getter->Invoke(node, {}));
			}
		}
	}
}

void InspectorPanel::CommitValue(ObjectID id, MethodBind const* setter, MethodBind const* getter, Variant const& value)
{
	Node* node = ObjectDatabase::GetInstance<Node>(id);
	if (id != m_cachedObjectID || node == nullptr || node->IsQueuedForDeletion() || node->GetSceneTree() != m_sceneTree
		|| setter == nullptr || getter == nullptr)
	{
		return;
	}

	Variant const oldValue = getter->Invoke(node, {});
	if (value.GetType() != oldValue.GetType() || value == oldValue)
	{
		return;
	}

	node = ObjectDatabase::GetInstance<Node>(id);
	if (node == nullptr || node->IsQueuedForDeletion())
	{
		return;
	}

	setter->Invoke(node, { value });
	node = ObjectDatabase::GetInstance<Node>(id);
	if (node == nullptr || node->IsQueuedForDeletion())
	{
		return;
	}

	Variant const actual = getter->Invoke(node, {});
	if (actual != oldValue && EditorNode::Get() != nullptr)
	{
		EditorNode::Get()->MarkSceneDirty();
	}

	RefreshValues();
}

void InspectorPanel::RebuildProperties(EditorUIContext& context)
{
	Node* node = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (node == nullptr || node->IsQueuedForDeletion() || node->GetSceneTree() != context.m_sceneTree)
	{
		return;
	}

	for (std::string const& className : m_inheritanceChain)
	{
		PropertyGroup group;
		group.m_className = className;
		auto* header      = MemNew<Label>();
		header->SetText(className);
		header->SetThemeTypeVariation("EditorInspectorTitle");
		m_content->AddNode(header);
		group.m_header = header;
		for (PropertyInfo const& prop : ClassDatabase::GetProperties(className))
		{
			if (!prop.HasUsage(PropertyInfo::UsageFlags::Inspector))
			{
				continue;
			}

			ObjectID const    id     = m_cachedObjectID;
			MethodBind const* setter = prop.GetSetter();
			MethodBind const* getter = prop.GetGetter();
			auto*             field  = InspectorProperty::Create(
				prop,
				className + "::" + prop.m_name,
				[this, id, setter, getter](Variant const& value)
				{
					CommitValue(id, setter, getter, value);
				});
			if (field == nullptr)
			{
				continue;
			}

			field->SetValueReader(
				[id, getter]() -> Variant
				{
					Node* target = ObjectDatabase::GetInstance<Node>(id);
					if (target == nullptr || target->IsQueuedForDeletion() || getter == nullptr)
					{
						return {};
					}

					return getter->Invoke(target, {});
				});
			m_content->AddNode(field->GetControl());
			group.m_properties.push_back({ field, getter });
		}

		m_propertyGroups.push_back(std::move(group));
	}

	RefreshValues();
	FilterProperties();
}

void InspectorPanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	AddNode(layout);
	layout->UpdateSize(GetSize());
	m_filter = MemNew<LineEdit>();
	m_filter->SetPlaceholder("Filter properties");
	m_filter->SetTextChangedCallback([this](String const&) { FilterProperties(); });
	layout->AddNode(m_filter);
	m_empty = EditorPopupUtils::AddLabel(layout, "No object selected");
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);
	m_content = MemNew<VBoxContainer>();
	m_content->SetSeparation(5.f);
	scroll->AddNode(m_content);
}

void InspectorPanel::FilterProperties()
{
	if (m_filter == nullptr)
	{
		return;
	}

	auto lower = [](std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		return text;
	};
	std::string const query = lower(m_filter->GetText().ToUtf8());
	for (PropertyGroup& group : m_propertyGroups)
	{
		bool any = false;
		for (PropertyEntry& entry : group.m_properties)
		{
			bool const visible = query.empty() || lower(entry.m_property->GetDisplayName()).find(query) != std::string::npos;
			entry.m_property->GetControl()->SetVisible(visible);
			any = any || visible;
		}

		group.m_header->SetVisible(any);
	}
}

void InspectorPanel::OnRender(EditorUIContext& context)
{
	m_sceneTree = context.m_sceneTree;
	ObjectID const id = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node* node = ObjectDatabase::GetInstance<Node>(id);
	bool const valid = node != nullptr && !node->IsQueuedForDeletion() && node->GetSceneTree() == m_sceneTree;
	if (!valid || id != m_cachedObjectID)
	{
		ClearProperties();
		m_cachedObjectID = valid ? id : ObjectID::Invalid;
		if (valid)
		{
			BuildInheritanceChain(node->GetClassName());
			RebuildProperties(context);
		}
	}

	m_empty->SetVisible(!valid);
	RefreshValues();
	FilterProperties();
}

void InspectorPanel::FinishInput(EditorUIContext& context)
{
	ObjectID const selected = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node* node = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (selected != m_cachedObjectID || node == nullptr || node->IsQueuedForDeletion())
	{
		ClearProperties();
		m_cachedObjectID = ObjectID::Invalid;
	}
}

void InspectorPanel::FinishEditing()
{
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			entry.m_property->EndEditing(LineEdit::EditEndReason::FocusLost);
		}
	}

	if (GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this);
	}
}

void InspectorPanel::OnClose()
{
	if (GetViewport() != nullptr)
	{
		GetViewport()->CancelGuiInteraction(this);
	}
}

Control* InspectorPanel::GetContentControl() const
{
	return m_content;
}
