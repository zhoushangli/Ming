#pragma once

#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

class InspectorPropertyInt final : public InspectorProperty
{
public:
	InspectorPropertyInt(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildNumeric();
	}
};
