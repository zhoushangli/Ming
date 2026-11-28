#pragma once

#include "MingEngine/Core/Object/Script.hpp"

class NativeScript : public Script
{
	MCLASS(NativeScript, Script)

public:
	bool MoveFrom(Resource&& other) override;

	bool Instantiate(Object* owner) override;
};
