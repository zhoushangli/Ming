#include "MingEngine/Core/Object/NativeScript.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

bool NativeScript::CopyFrom(Resource&& other)
{
	if (dynamic_cast<NativeScript*>(&other) == nullptr)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));
	return true;
}

bool NativeScript::Instantiate(Object* owner)
{
	if (owner == nullptr)
	{
		return false;
	}

	return g_engine->m_scriptSystem->GetOrCreateNativeManagedWrapper(owner) != nullptr;
}
