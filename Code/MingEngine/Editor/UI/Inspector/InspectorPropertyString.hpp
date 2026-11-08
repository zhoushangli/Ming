#pragma once

#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

class InspectorPropertyString final : public InspectorProperty
{
public:
	InspectorPropertyString(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildText();
	}
};
