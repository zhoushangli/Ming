#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

#include <utility>

class Script : public Resource
{
	MCLASS(Script, Resource)

public:
	Script()          = default;
	virtual ~Script() = default;

	bool MoveFrom(Resource&& other) override
	{
		if (dynamic_cast<Script*>(&other) == nullptr)
		{
			return false;
		}

		MoveBaseFrom(std::move(other));
		return true;
	}

protected:
	static void BindMethods() {};
};
