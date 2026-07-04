#pragma once

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyEuler final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<EulerAngles>())
		{
			return;
		}

		EulerAngles v = value.As<EulerAngles>();
		if (EditorUIWidgets::PropertyEuler(GetDisplayName(), m_labelId.c_str(), v))
		{
			EmitValueChanged(Variant(v));
		}
	}
};
