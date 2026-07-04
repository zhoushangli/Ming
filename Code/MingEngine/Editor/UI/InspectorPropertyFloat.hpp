#pragma once

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyFloat final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<float>())
		{
			return;
		}

		float f = value.As<float>();
		if (EditorUIWidgets::PropertyFloat(GetDisplayName(), m_labelId.c_str(), f))
		{
			EmitValueChanged(Variant(f));
		}
	}
};
