#pragma once

#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

class InspectorPropertyBool final : public InspectorProperty
{
public:
	InspectorPropertyBool(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildBool();
	}
};
