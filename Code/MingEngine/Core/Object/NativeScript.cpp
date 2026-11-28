#include "MingEngine/Core/Object/NativeScript.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Script/ScriptSystem.hpp"

bool NativeScript::MoveFrom(Resource&& other)
{
	auto* script = dynamic_cast<NativeScript*>(&other);
	if (script == nullptr)
	{
		return false;
	}

	m_name           = std::move(script->m_name);
	m_virtualPath    = std::move(script->m_virtualPath);
	m_sourceFilePath = std::move(script->m_sourceFilePath);
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
