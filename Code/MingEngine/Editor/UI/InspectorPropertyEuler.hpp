#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyEuler final : public InspectorProperty
{
public:
	InspectorPropertyEuler(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildComponents();
	}
};
