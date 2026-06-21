#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"

namespace
{
void DrawCenteredIcon(ImTextureID textureId, ImVec2 itemMin, ImVec2 itemMax, ImVec2 iconSize)
{
	if (textureId == ImTextureID{})
	{
		return;
	}

	ImVec2 const itemCenter((itemMin.x + itemMax.x) * 0.5f, (itemMin.y + itemMax.y) * 0.5f);
	ImVec2 const iconMin(itemCenter.x - iconSize.x * 0.5f, itemCenter.y - iconSize.y * 0.5f);
	ImVec2 const iconMax(iconMin.x + iconSize.x, iconMin.y + iconSize.y);
	EditorIcons::AddImage(ImGui::GetWindowDrawList(), textureId, iconMin, iconMax);
}

void DrawInspectorClassHeader(std::string const& className)
{
	constexpr float iconTextSpacing = 6.f;

	ImVec2 const iconSize = EditorUIStyle::InspectorHeaderIconSize();
	float const  headerHeight = EditorUIStyle::InspectorHeaderHeight();
	float const  availableWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 const headerMin = ImGui::GetCursorScreenPos();
	ImVec2 const headerMax(headerMin.x + availableWidth, headerMin.y + headerHeight);

	ImGui::InvisibleButton("##InspectorClassHeader", ImVec2(availableWidth, headerHeight));

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(headerMin, headerMax, ImGui::ColorConvertFloat4ToU32(EditorUIStyle::ControlBackgroundColor()), 3.f);

	ImTextureID const textureId = EditorIcons::GetClassIconId(className);
	ImVec2 const     textSize = ImGui::CalcTextSize(className.c_str());
	float const      contentWidth = iconSize.x + iconTextSpacing + textSize.x;
	float const      contentX = headerMin.x + (availableWidth - contentWidth) * 0.5f;
	float const      iconY = headerMin.y + (headerHeight - iconSize.y) * 0.5f;
	float const      textY = headerMin.y + (headerHeight - textSize.y) * 0.5f;

	if (textureId != ImTextureID{})
	{
		EditorIcons::AddImage(
			drawList,
			textureId,
			ImVec2(contentX, iconY),
			ImVec2(contentX + iconSize.x, iconY + iconSize.y));
	}

	ImU32 const textColor = ImGui::GetColorU32(ImGuiCol_Text);
	ImVec2 const textPos(contentX + iconSize.x + iconTextSpacing, textY);
	drawList->AddText(textPos, textColor, className.c_str());
	drawList->AddText(ImVec2(textPos.x + 1.f, textPos.y), textColor, className.c_str());
}
} // namespace

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
	ImVec2 const iconSize = EditorUIStyle::InspectorTabIconSize();
	ImVec2 const buttonSize = EditorUIStyle::InspectorTabButtonSize();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.f, 4.f));
	ImGui::PushStyleColor(ImGuiCol_Button, EditorUIStyle::ControlBackgroundColor());
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, EditorUIStyle::ControlBackgroundHoveredColor());
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, EditorUIStyle::ControlBackgroundActiveColor());

	for (size_t i = 0; i < m_inheritanceChain.size(); ++i)
	{
		std::string const& className = m_inheritanceChain[i];
		bool const         isSelected = i == m_activeTabIndex;

		if (i > 0)
		{
			ImGui::SameLine();
		}

		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, EditorUIStyle::ControlBackgroundActiveColor());
		}

		ImTextureID const textureId = EditorIcons::GetClassIconId(className);
		std::string const buttonId = "##ClassIconTab_" + className;
		bool const clicked = ImGui::Button(buttonId.c_str(), buttonSize);
		DrawCenteredIcon(textureId, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), iconSize);

		if (isSelected)
		{
			ImGui::PopStyleColor();
		}

		if (clicked)
		{
			m_activeTabIndex = i;
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", className.c_str());
		}
	}

	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
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
	if (m_activeTabIndex < m_inheritanceChain.size())
	{
		std::string const& className = m_inheritanceChain[m_activeTabIndex];
		DrawInspectorClassHeader(className);
		ImGui::Separator();
	}

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

