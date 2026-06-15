#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"

InspectorPanel::InspectorPanel() : EditorPanel("Inspector") {}

InspectorPanel::~InspectorPanel()
{
	for (InspectorProperty* p : m_properties)
	{
		delete p;
	}
	m_properties.clear();
}

// ——— Selection & inheritance chain ———

void InspectorPanel::BuildInheritanceChain(std::string const& className)
{
	m_inheritanceChain.clear();
	std::string current = className;

	while (true)
	{
		m_inheritanceChain.push_back(current);
		if (current == "Node")
		{
			break;
		}

		ClassInfo const* info = ClassDatabase::GetClassInfo(current);
		if (info == nullptr || info->m_parentClassName.empty() || info->m_parentClassName == current)
		{
			break;
		}
		current = info->m_parentClassName;
	}
}

void InspectorPanel::RebuildProperties(EditorUIContext& context)
{
	// 1) Clean up old properties
	for (InspectorProperty* p : m_properties)
	{
		delete p;
	}
	m_properties.clear();

	// 2) Resolve node
	Node* node = context.m_sceneTree->ResolveNode(m_cachedHandle);
	if (node == nullptr)
	{
		return;
	}

	// 3) Get properties for the active tab class
	if (m_activeTabIndex >= m_inheritanceChain.size())
	{
		return;
	}

	std::string const& className = m_inheritanceChain[m_activeTabIndex];
	std::vector<PropertyInfo> properties = ClassDatabase::GetProperties(className);

	for (PropertyInfo& prop : properties)
	{
		if (!prop.HasUsage(PropertyInfo::UsageFlags::Inspector))
		{
			continue;
		}

		// Get current value via getter
		Variant value;
		MethodBind const* getter = prop.GetGetter();
		if (getter != nullptr)
		{
			value = getter->Invoke(*node, {});
		}

		std::string labelId = "##" + className + "::" + prop.m_name;

		InspectorProperty* ip = InspectorProperty::Create(std::move(prop), std::move(value), std::move(labelId));
		if (ip != nullptr)
		{
			m_properties.push_back(ip);
		}
	}
}

// ——— Tab bar ———

void InspectorPanel::RenderTabBar()
{
	if (ImGui::BeginTabBar("##ClassTabs", ImGuiTabBarFlags_AutoSelectNewTabs))
	{
		for (size_t i = 0; i < m_inheritanceChain.size(); ++i)
		{
			std::string tabLabel = m_inheritanceChain[i] + "##tab";
			if (ImGui::BeginTabItem(tabLabel.c_str()))
			{
				m_activeTabIndex = i;
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}
}

// ——— Main render ———

void InspectorPanel::OnRender(EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());

	// 1) Resolve selection
	if (context.m_selection == nullptr || context.m_sceneTree == nullptr)
	{
		ImGui::TextUnformatted("No editor context");
		ImGui::End();
		return;
	}

	NodeHandle handle = context.m_selection->GetSelected();
	if (!handle.IsValid())
	{
		ImGui::TextUnformatted("No object selected");
		ImGui::End();
		return;
	}

	Node* node = context.m_sceneTree->ResolveNode(handle);
	if (node == nullptr)
	{
		m_cachedHandle = NodeHandle();
		ImGui::TextUnformatted("Selected object not found");
		ImGui::End();
		return;
	}

	// 2) Detect selection change
	bool const selectionChanged = (m_cachedHandle != handle);
	if (selectionChanged)
	{
		m_cachedHandle   = handle;
		m_activeTabIndex = 0;
		BuildInheritanceChain(node->GetClassName());
	}

	// 3) Tab switch detection
	size_t const previousTabIndex = m_activeTabIndex;

	// 4) Render class tab bar
	if (!m_inheritanceChain.empty())
	{
		RenderTabBar();
		ImGui::Separator();
	}

	// Clamp tab index
	if (m_activeTabIndex >= m_inheritanceChain.size())
	{
		m_activeTabIndex = 0;
	}

	// 5) Rebuild property list when selection or tab changes
	if (selectionChanged || m_activeTabIndex != previousTabIndex)
	{
		RebuildProperties(context);
	}

	// 6) Render properties — each InspectorProperty owns its full layout
	if (m_properties.empty())
	{
		if (m_activeTabIndex < m_inheritanceChain.size())
		{
			ImGui::TextUnformatted(
				("No editable properties for " + m_inheritanceChain[m_activeTabIndex]).c_str());
		}
	}
	else
	{
		for (InspectorProperty* prop : m_properties)
		{
			prop->Render();
			prop->Apply(node);
		}
	}

	ImGui::End();
}

