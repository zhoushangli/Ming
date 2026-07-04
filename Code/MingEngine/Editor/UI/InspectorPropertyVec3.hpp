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
		if (!value.Is<Vec3>())
		{
			return;
		}

		Vec3 v = value.As<Vec3>();
		bool const edited = EditorUIWidgets::PropertyVec3(GetDisplayName(), m_labelId.c_str(), v);

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
