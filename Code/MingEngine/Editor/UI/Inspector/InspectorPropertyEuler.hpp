#pragma once

#include "MingEngine/Editor/UI/Inspector/InspectorProperty.hpp"

class InspectorPropertyEuler final : public InspectorProperty
{
public:
	InspectorPropertyEuler(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildComponents();
	}
};
