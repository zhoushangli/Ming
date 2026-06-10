#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "ThirdParty/imgui/imgui.h"

class InspectorPropertyFloat final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void Render() override
	{
		float f = m_value.As<float>();
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();
		if (ImGui::DragFloat(m_labelId.c_str(), &f, 0.1f, 0.0f, 0.0f, "%.2f"))
		{
			m_value  = Variant(f);
			m_edited = true;
		}
		ImGui::Columns(1);
	}
};
