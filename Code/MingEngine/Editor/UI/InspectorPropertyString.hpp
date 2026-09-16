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
		if (!value.Is<String>())
		{
			return;
		}

		std::string str = value.As<String>().ToUtf8();
		if (EditorUIWidgets::PropertyString(GetDisplayName(), m_labelId.c_str(), str))
		{
			EmitValueChanged(Variant(str));
		}
	}
};
