#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyResource final : public InspectorProperty
{
public:
	InspectorPropertyResource(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildText(true);
	}
};
