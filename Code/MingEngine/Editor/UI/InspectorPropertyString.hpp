#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "ThirdParty/imgui/imgui.h"
#include "ThirdParty/imgui/misc/cpp/imgui_stdlib.h"

class InspectorPropertyString final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void Render() override
	{
		std::string& str = m_value.As<std::string>();
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();
		ImGui::InputText(m_labelId.c_str(), &str);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			m_edited = true;
		}
		ImGui::Columns(1);
	}
};
