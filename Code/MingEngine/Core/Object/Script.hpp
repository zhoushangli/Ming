#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

#include <utility>

class ScriptInstance;

class Script : public Resource
{
	MCLASS(Script, Resource)

public:
	Script()          = default;
	virtual ~Script() = default;

	bool CopyFrom(Resource&& other) override;

	virtual bool Instantiate(Object* owner) = 0;

protected:
	static void BindMethods() {};
};
