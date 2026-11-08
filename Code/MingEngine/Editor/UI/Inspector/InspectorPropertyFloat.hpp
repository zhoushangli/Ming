#pragma once

#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

class InspectorPropertyFloat final : public InspectorProperty
{
public:
	InspectorPropertyFloat(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildNumeric();
	}
};
