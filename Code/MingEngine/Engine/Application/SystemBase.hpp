#pragma once

#include "MingEngine/Core/Object/Object.hpp"

class SystemBase : public Object
{
	MCLASS(SystemBase, Object)

public:
	virtual ~SystemBase() = default;

	virtual void Startup() {}
	virtual void Shutdown() {}
	virtual void BeginFrame() {}
	virtual void EndFrame() {}

	static void BindMethods();
};
