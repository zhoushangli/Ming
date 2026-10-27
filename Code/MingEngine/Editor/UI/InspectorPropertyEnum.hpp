#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyEnum final : public InspectorProperty
{
public:
	InspectorPropertyEnum(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildEnum();
	}
};
