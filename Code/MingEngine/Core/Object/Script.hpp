#pragma once

#include "MingEngine/Core/Object/Resource.hpp"

class Script : public Resource
{
	MCLASS(Script, Resource)

public:
	Script()          = default;
	virtual ~Script() = default;

	bool CopyFrom([[maybe_unused]] Resource const& other) override { return false; }

protected:
	static void BindMethods() {};
};