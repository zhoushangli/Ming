#pragma once

#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include <cmath>

class InspectorPropertyVec3 final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		(void)context;
		if (!value.Is<Vector3>())
		{
			return;
		}

		Vector3       v            = value.As<Vector3>();
		Vector3 const defaultValue = m_info.m_name == "scale" ? Vector3::One : Vector3::Zero;
		bool const    edited = EditorUIWidgets::PropertyVec3(GetDisplayName(), m_labelId.c_str(), v, defaultValue);

		if (m_info.m_name == "scale")
		{
			if (std::abs(v.x) < 1e-5f)
				v.x = 1e-5f;
			if (std::abs(v.y) < 1e-5f)
				v.y = 1e-5f;
			if (std::abs(v.z) < 1e-5f)
				v.z = 1e-5f;
		}

		if (edited)
		{
			EmitValueChanged(Variant(v));
		}
	}
};
