#include "MingEngine/Editor/UI/InspectorPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Scene/Core/ClassDatabase.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>

InspectorPanel::InspectorPanel() : EditorPanel("Inspector") {}

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

		ClassDatabase::ClassInfo const* info = ClassDatabase::GetClassInfo(current);
		if (info == nullptr || info->m_parentClassName.empty() || info->m_parentClassName == current)
		{
			break;
		}
		current = info->m_parentClassName;
	}
}

void InspectorPanel::RefreshPropertyValues(EditorUIContext& context, std::string const& className)
{
	// Ensure a cache entry exists for this class
	m_propertyValuesByClass[className].clear();

	Node* node = context.m_sceneTree->ResolveNode(m_cachedHandle);
	if (node == nullptr)
	{
		return;
	}

	std::vector<ClassDatabase::PropertyInfo> properties = ClassDatabase::GetProperties(className);
	for (ClassDatabase::PropertyInfo const& prop : properties)
	{
		if (!prop.HasUsage(ClassDatabase::PropertyInfo::PropertyUsageFlags::Editor))
		{
			continue;
		}

		MethodBind const* getter = prop.GetGetter();
		if (getter != nullptr)
		{
			m_propertyValuesByClass[className].push_back(getter->Invoke(*node, {}));
		}
		else
		{
			m_propertyValuesByClass[className].push_back(Variant());
		}
	}
}

// ——— Display helpers ———

std::string InspectorPanel::SnakeToTitle(std::string const& snake)
{
	std::string result;
	result.reserve(snake.size());
	bool capitalize = true;

	for (char ch : snake)
	{
		if (ch == '_')
		{
			result += ' ';
			capitalize = true;
		}
		else if (capitalize)
		{
			result += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
			capitalize = false;
		}
		else
		{
			result += ch;
		}
	}

	return result;
}

// ——— Tab bar ———

void InspectorPanel::RenderTabBar()
{
	// Render as a proper tab bar so tabs look and behave like Godot's
	if (ImGui::BeginTabBar("##ClassTabs", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs))
	{
		for (size_t i = 0; i < m_inheritanceChain.size(); ++i)
		{
			std::string tabLabel = m_inheritanceChain[i] + "##tab";
			if (ImGui::BeginTabItem(tabLabel.c_str()))
			{
				// This tab is selected
				m_activeTabIndex = i;
				ImGui::EndTabItem();
			}
		}

		ImGui::EndTabBar();
	}
}

// ——— Property rendering ———

void InspectorPanel::RenderProperty(
	ClassDatabase::PropertyInfo const& prop,
	Variant& value,
	EditorUIContext& context,
	std::string const& className,
	bool showLabel)
{
	std::string const displayName = SnakeToTitle(prop.m_name);
	std::string const labelId     = "##" + className + "::" + prop.m_name;

	Node* node               = context.m_sceneTree->ResolveNode(m_cachedHandle);
	MethodBind const* setter = prop.GetSetter();
	bool edited              = false;

	switch (prop.m_type)
	{
	case Variant::Type::Bool:
	{
		if (value.Is<bool>())
		{
			bool b = value.As<bool>();
			edited = ImGui::Checkbox(labelId.c_str(), &b);
			if (edited)
			{
				value = Variant(b);
			}
		}
		else
		{
			if (showLabel)
			{
				ImGui::TextUnformatted(displayName.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted("(type mismatch)");
			}
			else
			{
				ImGui::TextUnformatted("(type mismatch)");
			}
		}
		break;
	}
	case Variant::Type::Int:
	{
		if (value.Is<int>())
		{
			int i  = value.As<int>();
			edited = ImGui::DragInt(labelId.c_str(), &i, 1.0f);
			if (edited)
			{
				value = Variant(i);
			}
		}
		else
		{
			if (showLabel)
			{
				ImGui::TextUnformatted(displayName.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted("(type mismatch)");
			}
			else
			{
				ImGui::TextUnformatted("(type mismatch)");
			}
		}
		break;
	}
	case Variant::Type::Float:
	{
		if (value.Is<float>())
		{
			float f = value.As<float>();
			edited  = ImGui::DragFloat(labelId.c_str(), &f, 0.1f, 0.0f, 0.0f, "%.2f");
			if (edited)
			{
				value = Variant(f);
			}
		}
		else
		{
			if (showLabel)
			{
				ImGui::TextUnformatted(displayName.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted("(type mismatch)");
			}
			else
			{
				ImGui::TextUnformatted("(type mismatch)");
			}
		}
		break;
	}
	case Variant::Type::String:
	{
		if (value.Is<std::string>())
		{
			std::string& str = value.As<std::string>();
			ImGui::InputText(labelId.c_str(), &str);
			edited = ImGui::IsItemDeactivatedAfterEdit();
		}
		else
		{
			if (showLabel)
			{
				ImGui::TextUnformatted(displayName.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted("(type mismatch)");
			}
			else
			{
				ImGui::TextUnformatted("(type mismatch)");
			}
		}
		break;
	}
	case Variant::Type::Vec3:
	{
		if (value.Is<Vec3>())
		{
			Vec3 v = value.As<Vec3>();
			// Render three separate floats with colored x/y/z labels
			ImGui::PushID(labelId.c_str());
			// Use three equal columns so each component fills one third of the cell
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemSpacing);
			ImGui::Columns(3, nullptr, false);

			// X (red)
			ImGui::TextColored(ImVec4(0.9f, 0.25f, 0.25f, 1.0f), "x");
			ImGui::SameLine();
			ImGui::PushItemWidth(-FLT_MIN);
			edited = ImGui::DragFloat("##x", &v.x, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			// Y (green)
			ImGui::TextColored(ImVec4(0.35f, 0.8f, 0.35f, 1.0f), "y");
			ImGui::SameLine();
			ImGui::PushItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##y", &v.y, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			// Z (blue)
			ImGui::TextColored(ImVec4(0.35f, 0.5f, 0.9f, 1.0f), "z");
			ImGui::SameLine();
			ImGui::PushItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##z", &v.z, 0.1f, 0.0f, 0.0f, "%.2f");
			ImGui::PopItemWidth();
			ImGui::Columns(1);
			ImGui::PopStyleVar();
			ImGui::PopID();

			// Clamp scale components
			if (prop.m_name == "scale")
			{
				if (std::abs(v.x) < 1e-5f)
					v.x = 1e-5f;
				if (std::abs(v.y) < 1e-5f)
					v.y = 1e-5f;
				if (std::abs(v.z) < 1e-5f)
					v.z = 1e-5f;
			}

			if (edited)
			{
				value = Variant(v);
			}
		}
		else
		{
			if (showLabel)
			{
				ImGui::TextUnformatted(displayName.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted("(type mismatch)");
			}
			else
			{
				ImGui::TextUnformatted("(type mismatch)");
			}
		}
		break;
	}
	case Variant::Type::EulerAngles:
	{
		if (value.Is<EulerAngles>())
		{
			EulerAngles e = value.As<EulerAngles>();
			// Render as three colored components similar to Vec3
			ImGui::PushID(labelId.c_str());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemSpacing);
			ImGui::Columns(3, nullptr, false);

			ImGui::TextColored(ImVec4(0.9f, 0.25f, 0.25f, 1.0f), "x");
			ImGui::SameLine();
			ImGui::PushItemWidth(-FLT_MIN);
			edited = ImGui::DragFloat("##yaw", &e.m_yawDegrees, 0.1f, 0.0f, 0.0f, "%.1f");
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::TextColored(ImVec4(0.35f, 0.8f, 0.35f, 1.0f), "y");
			ImGui::SameLine();
			ImGui::PushItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##pitch", &e.m_pitchDegrees, 0.1f, 0.0f, 0.0f, "%.1f");
			ImGui::PopItemWidth();
			ImGui::NextColumn();

			ImGui::TextColored(ImVec4(0.35f, 0.5f, 0.9f, 1.0f), "z");
			ImGui::SameLine();
			ImGui::PushItemWidth(-FLT_MIN);
			edited |= ImGui::DragFloat("##roll", &e.m_rollDegrees, 0.1f, 0.0f, 0.0f, "%.1f");
			ImGui::PopItemWidth();
			ImGui::Columns(1);
			ImGui::PopStyleVar();
			ImGui::PopID();

			if (edited)
			{
				value = Variant(e);
			}
		}
		else
		{
			if (showLabel)
			{
				ImGui::TextUnformatted(displayName.c_str());
				ImGui::SameLine();
				ImGui::TextUnformatted("(type mismatch)");
			}
			else
			{
				ImGui::TextUnformatted("(type mismatch)");
			}
		}
		break;
	}
	case Variant::Type::Matrix4x4:
	{
		ImGui::TextUnformatted(displayName.c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted("(Matrix4x4)");
		break;
	}
	default:
	{
		ImGui::TextUnformatted(displayName.c_str());
		ImGui::SameLine();
		ImGui::TextUnformatted("(unsupported type)");
		break;
	}
	}

	// Write back through setter and refresh from getter to keep cache in sync
	if (edited && setter != nullptr && node != nullptr)
	{
		setter->Invoke(*node, { value });

		// Refresh value from getter if available
		MethodBind const* getter = prop.GetGetter();
		if (getter != nullptr)
		{
			Variant refreshed = getter->Invoke(*node, {});
			value             = refreshed;
		}
	}
}

// ——— Main render ———

void InspectorPanel::OnRender(EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());

	// 1. Resolve selection
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

	// 2. Detect selection change
	bool const selectionChanged = (m_cachedHandle != handle);
	if (selectionChanged)
	{
		m_cachedHandle   = handle;
		m_activeTabIndex = 0;
		m_propertyValuesByClass.clear();
		BuildInheritanceChain(node->GetClassName());
	}

	// 3. Tab switch detection
	size_t const previousTabIndex = m_activeTabIndex;

	// 4. Render class tab bar
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

	// 5. Refresh property values when selection or tab changes
	if (selectionChanged || m_activeTabIndex != previousTabIndex)
	{
		if (m_activeTabIndex < m_inheritanceChain.size())
		{
			RefreshPropertyValues(context, m_inheritanceChain[m_activeTabIndex]);
		}
	}

	// 6. Render properties
	if (m_activeTabIndex < m_inheritanceChain.size())
	{
		std::vector<ClassDatabase::PropertyInfo> properties =
			ClassDatabase::GetProperties(m_inheritanceChain[m_activeTabIndex]);

		// Filter to editor-only and count
		std::vector<ClassDatabase::PropertyInfo const*> editorProps;
		for (ClassDatabase::PropertyInfo const& p : properties)
		{
			if (p.HasUsage(ClassDatabase::PropertyInfo::PropertyUsageFlags::Editor))
			{
				editorProps.push_back(&p);
			}
		}

		if (editorProps.empty())
		{
			ImGui::TextUnformatted(("No editable properties for " + m_inheritanceChain[m_activeTabIndex]).c_str());
		}
		else
		{
			std::string const& className = m_inheritanceChain[m_activeTabIndex];
			auto it                      = m_propertyValuesByClass.find(className);
			if (it != m_propertyValuesByClass.end() && it->second.size() == editorProps.size())
			{
				// Render as two-column table: name | control
				if (ImGui::BeginTable("##PropsTable", 2, ImGuiTableFlags_SizingStretchSame))
				{
					for (size_t i = 0; i < editorProps.size(); ++i)
					{
						// Place complex vector-like properties on their own second row
						bool isVec3  = (editorProps[i]->m_type == Variant::Type::Vec3);
						bool isEuler = (editorProps[i]->m_type == Variant::Type::EulerAngles);

						if (isVec3 || isEuler)
						{
							// First row: property name, empty control cell
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(SnakeToTitle(editorProps[i]->m_name).c_str());
							ImGui::TableNextColumn();

							// Second row: empty name cell, controls in control cell
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							RenderProperty(*editorProps[i], it->second[i], context, className, false);
						}
						else
						{
							ImGui::TableNextRow();
							ImGui::TableNextColumn();
							ImGui::TextUnformatted(SnakeToTitle(editorProps[i]->m_name).c_str());
							ImGui::TableNextColumn();
							RenderProperty(*editorProps[i], it->second[i], context, className, false);
						}
					}

					ImGui::EndTable();
				}
			}
		}
	}

	ImGui::End();
}
