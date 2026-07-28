#pragma once

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyInt final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<int>())
		{
			return;
		}

		int i = value.As<int>();
		if (EditorUIWidgets::PropertyInt(GetDisplayName(), m_labelId.c_str(), i))
		{
			EmitValueChanged(Variant(i));
		}
	}
};
