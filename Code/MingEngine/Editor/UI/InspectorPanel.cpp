#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"

namespace
{
void DrawInspectorClassHeader(std::string const& className)
{
	constexpr float iconTextSpacing = 6.f;

	ImVec2 const iconSize       = EditorUIStyle::InspectorHeaderIconSize();
	float const  headerHeight   = EditorUIStyle::InspectorHeaderHeight();
	float const  availableWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 const headerMin      = ImGui::GetCursorScreenPos();
	ImVec2 const headerMax(headerMin.x + availableWidth, headerMin.y + headerHeight);
	std::string const headerId = "##InspectorClassHeader_" + className;

	ImGui::InvisibleButton(headerId.c_str(), ImVec2(availableWidth, headerHeight));

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(
		headerMin,
		headerMax,
		ImGui::ColorConvertFloat4ToU32(EditorUIStyle::ControlBackgroundColor()),
		3.f);

	ImTextureID const textureId    = EditorIcons::GetClassIconId(className);
	ImVec2 const      textSize     = ImGui::CalcTextSize(className.c_str());
	float const       contentWidth = iconSize.x + iconTextSpacing + textSize.x;
	float const       contentX     = headerMin.x + (availableWidth - contentWidth) * 0.5f;
	float const       iconY        = headerMin.y + (headerHeight - iconSize.y) * 0.5f;
	float const       textY        = headerMin.y + (headerHeight - textSize.y) * 0.5f;

	if (textureId != ImTextureID{})
	{
		EditorIcons::AddImage(
			drawList,
			textureId,
			ImVec2(contentX, iconY),
			ImVec2(contentX + iconSize.x, iconY + iconSize.y));
	}

	ImU32 const  textColor = ImGui::GetColorU32(ImGuiCol_Text);
	ImVec2 const textPos(contentX + iconSize.x + iconTextSpacing, textY);
	drawList->AddText(textPos, textColor, className.c_str());
	drawList->AddText(ImVec2(textPos.x + 1.f, textPos.y), textColor, className.c_str());
}
} // namespace

InspectorPanel::InspectorPanel() : EditorPanel("Inspector") {}

InspectorPanel::~InspectorPanel()
{
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (InspectorProperty* p : group.m_properties)
		{
			delete p;
		}
	}
	m_propertyGroups.clear();
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
	for (PropertyGroup& group : m_propertyGroups)
	{
		for (InspectorProperty* p : group.m_properties)
		{
			delete p;
		}
	}
	m_propertyGroups.clear();

	Node* node = context.m_sceneTree->ResolveNode(m_cachedHandle);
	if (node == nullptr)
	{
		return;
	}

	for (std::string const& className : m_inheritanceChain)
	{
		PropertyGroup group;
		group.m_className = className;

		std::vector<PropertyInfo> properties = ClassDatabase::GetProperties(className);
		for (PropertyInfo& prop : properties)
		{
			if (!prop.HasUsage(PropertyInfo::UsageFlags::Inspector))
			{
				continue;
			}

			std::string                             labelId   = "##" + className + "::" + prop.m_name;
			MethodBind const*                       setter    = prop.GetSetter();
			InspectorProperty::ValueChangedCallback onChanged = [node, setter](Variant const& value)
			{
				if (node != nullptr && setter != nullptr)
				{
					setter->Invoke(node, { value });
				}
			};

			InspectorProperty* ip =
				InspectorProperty::Create(std::move(prop), node, std::move(labelId), std::move(onChanged));
			if (ip != nullptr)
			{
				group.m_properties.push_back(ip);
			}
		}

		if (!group.m_properties.empty())
		{
			m_propertyGroups.push_back(std::move(group));
		}
	}
}

// ——— Tab bar ———

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
		m_cachedHandle = handle;
		BuildInheritanceChain(node->GetClassName());
	}

	// 3) Rebuild property list when selection changes
	if (selectionChanged)
	{
		RebuildProperties(context);
	}

	// 6) Render properties — each InspectorProperty owns its full layout
	if (m_propertyGroups.empty())
	{
		ImGui::TextUnformatted("No editable properties");
	}
	else
	{
		bool isFirstGroup = true;
		for (PropertyGroup& group : m_propertyGroups)
		{
			if (!isFirstGroup)
			{
				ImGui::Dummy(ImVec2(0.f, 8.f));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0.f, 8.f));
			}

			DrawInspectorClassHeader(group.m_className);
			ImGui::Dummy(ImVec2(0.f, 4.f));

			for (InspectorProperty* prop : group.m_properties)
			{
				prop->Render(context);
			}

			isFirstGroup = false;
		}
	}

	ImGui::End();
}
