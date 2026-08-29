#include "MingEngine/Core/Object/Script.hpp"

#include "MingEngine/Core/Object/ScriptInstance.hpp"

bool Script::CopyFrom(Resource&& other)
{
	if (dynamic_cast<Script*>(&other) == nullptr)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));
	return true;
}
