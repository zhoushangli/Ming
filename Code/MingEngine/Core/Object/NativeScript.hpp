#pragma once

#include "MingEngine/Core/Object/Script.hpp"

class NativeScript : public Script
{
	MCLASS(NativeScript, Script)

public:
	bool CopyFrom(Resource&& other) override;

	bool Instantiate(Object* owner) override;
};

