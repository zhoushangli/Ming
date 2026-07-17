#pragma once

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyColor final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<Color>())
		{
			return;
		}

		Color color = value.As<Color>();
		if (EditorUIWidgets::PropertyColor(GetDisplayName(), m_labelId.c_str(), color))
		{
			EmitValueChanged(Variant(color));
		}
	}
};
