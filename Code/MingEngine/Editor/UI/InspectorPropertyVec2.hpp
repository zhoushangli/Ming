#pragma once

#include "MingEngine/Editor/UI/InspectorProperty.hpp"

class InspectorPropertyVec2 final : public InspectorProperty
{
public:
	InspectorPropertyVec2(PropertyInfo info, std::string labelId, ValueChangedCallback changed)
		: InspectorProperty(std::move(info), std::move(labelId), std::move(changed))
	{
		BuildComponents();
	}
};
