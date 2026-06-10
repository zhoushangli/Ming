#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "ThirdParty/imgui/imgui.h"

class InspectorPropertyInt final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void Render() override
	{
		int i = m_value.As<int>();
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();
		if (ImGui::DragInt(m_labelId.c_str(), &i, 1.0f))
		{
			m_value  = Variant(i);
			m_edited = true;
		}
		ImGui::Columns(1);
	}
};
