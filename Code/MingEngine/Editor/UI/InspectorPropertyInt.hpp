#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "ThirdParty/imgui/imgui.h"

class InspectorPropertyInt final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		int i = value.As<int>();
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();
		if (ImGui::DragInt(m_labelId.c_str(), &i, 1.0f))
		{
			EmitValueChanged(Variant(i));
		}
		ImGui::Columns(1);
	}
};
