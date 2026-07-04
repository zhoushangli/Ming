#pragma once

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyBool final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<bool>())
		{
			return;
		}

		bool b = value.As<bool>();
		if (EditorUIWidgets::PropertyBool(GetDisplayName(), m_labelId.c_str(), b))
		{
			EmitValueChanged(Variant(b));
		}
	}
};
