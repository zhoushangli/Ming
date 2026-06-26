#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

class Script : public Resource
{
	MCLASS(Script, Resource)

public:
	Script()          = default;
	virtual ~Script() = default;

protected:
	static void BindMethods() {};
};