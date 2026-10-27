#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "ThirdParty/imgui/imgui.h"

InspectorPanel::InspectorPanel() : EditorPanel("Inspector")
{
}

InspectorPanel::~InspectorPanel()
{
	ClearProperties();
}

void InspectorPanel::ClearProperties()
{
	m_host.Interrupt();
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (PropertyEntry& entry : group.m_properties)
		{
			delete entry.m_property;
		}
	}

	m_propertyGroups.clear();
	if (VBoxContainer* content = m_host.GetContent(); content != nullptr && m_host.GetViewport() != nullptr)
	{
		std::vector<Node*> const children = content->GetChildren();
		for (Node* child : children)
		{
			if (!child->IsQueuedForDeletion())
			{
				if (auto* control = dynamic_cast<Control*>(child))
				{
					control->SetVisible(false);
				}

				child->QueueFree();
			}
		}
	}
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
		header->SetFontSize(18);
		m_host.GetContent()->AddNode(header);
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
			m_host.GetContent()->AddNode(field->GetControl());
			group.m_properties.push_back({ field, getter });
		}

		m_propertyGroups.push_back(std::move(group));
	}

	RefreshValues();
}

void InspectorPanel::OnRender(EditorUIContext& context)
{
	if (!ImGui::Begin(GetTitle(), GetOpenState(), ImGuiWindowFlags_NoNavInputs))
	{
		ImGui::End();
		return;
	}

	m_sceneTree = context.m_sceneTree;
	m_host.Initialize(context.m_sceneTree);
	ObjectID const id   = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node*          node = ObjectDatabase::GetInstance<Node>(id);
	bool const valid = node != nullptr && !node->IsQueuedForDeletion() && node->GetSceneTree() == context.m_sceneTree;
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

	if (!valid)
	{
		ImGui::TextUnformatted("No object selected");
		ImGui::End();
		return;
	}

	RefreshValues();
	ImVec2 const available = ImGui::GetContentRegionAvail();
	m_host.Draw(Vector2(available.x, available.y));

	// 1) Translate the existing editor drag payload into a native resource field assignment.
	if (ImGui::BeginDragDropTarget())
	{
		EditorNode* editor = EditorNode::Get();
		VirtualPath path;
		if (editor != nullptr && m_host.GetViewport()->GetGuiPopup() == nullptr && editor->m_dragDrop.TryGetData(path))
		{
			ImVec2 const  mouse    = ImGui::GetMousePos();
			Vector2 const position = m_host.ToLocal(Vector2(mouse.x, mouse.y));
			for (PropertyGroup& group : m_propertyGroups)
			{
				for (PropertyEntry& entry : group.m_properties)
				{
					if (entry.m_property->Contains(position) && entry.m_property->CanAssignResource(path))
					{
						editor->m_dragDrop.AllowDrop();
						if (ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadType) != nullptr)
						{
							entry.m_property->AssignResource(path);
						}
					}
				}
			}
		}

		ImGui::EndDragDropTarget();
	}

	ImGui::End();
}

void InspectorPanel::FinishInput(EditorUIContext& context)
{
	ObjectID const selected = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node*          node     = ObjectDatabase::GetInstance<Node>(m_cachedObjectID);
	if (selected != m_cachedObjectID || node == nullptr || node->IsQueuedForDeletion())
	{
		ClearProperties();
		m_cachedObjectID = ObjectID::Invalid;
		return;
	}

	m_host.FinishOutsideClick();
}

void InspectorPanel::RenderNative(EditorUIContext& context, Vector2 origin, Vector2 size)
{
	m_sceneTree = context.m_sceneTree;
	m_host.Initialize(context.m_sceneTree);
	ObjectID const id   = context.m_selection != nullptr ? context.m_selection->GetSelected() : ObjectID::Invalid;
	Node*          node = ObjectDatabase::GetInstance<Node>(id);
	bool const valid = node != nullptr && !node->IsQueuedForDeletion() && node->GetSceneTree() == context.m_sceneTree;
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

	RefreshValues();
	m_host.DrawNative(origin, size);
	m_host.GetRootControl()->SetVisible(valid);
}
