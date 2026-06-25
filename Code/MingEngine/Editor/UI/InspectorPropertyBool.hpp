#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "ThirdParty/imgui/imgui.h"

class InspectorPropertyBool final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(Variant const& value) override
	{
		bool b = value.As<bool>();
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();
		if (ImGui::Checkbox(m_labelId.c_str(), &b))
		{
			EmitValueChanged(Variant(b));
		}
		ImGui::Columns(1);
	}
};
