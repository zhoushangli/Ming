#pragma once

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyString final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<std::string>())
		{
			return;
		}

		std::string str = value.As<std::string>();
		if (EditorUIWidgets::PropertyString(GetDisplayName(), m_labelId.c_str(), str))
		{
			EmitValueChanged(Variant(str));
		}
	}
};
