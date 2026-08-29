#pragma once

#include "MingEngine/Core/Object/Script.hpp"

class Object;

// In ScriptInstance, you may see a lot of werid way to write code
// like many reldundant std::move and std::exchange
// This is because C++ and C# script instance is a two-way binding
// which means the they deconstruct, they may call each other in a recursive way
// So we need to make sure the script clear itself first before calling the other side
class ScriptInstance
{
public:
	ScriptInstance() = default;
	virtual ~ScriptInstance() {}

	ScriptInstance(ScriptInstance const&)            = delete;
	ScriptInstance& operator=(ScriptInstance const&) = delete;
	ScriptInstance(ScriptInstance&&)                 = delete;
	ScriptInstance& operator=(ScriptInstance&&)      = delete;

	virtual Object*     GetOwner() const { return nullptr; }
	virtual Ref<Script> GetScript() const           = 0;
	virtual bool        ReloadGCHandle(void* value) = 0;

	virtual void Notification(int notification, bool reverse) = 0;
};