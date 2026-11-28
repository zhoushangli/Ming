#include "MingEngine/Core/Object/Script.hpp"

#include "MingEngine/Core/Object/ScriptInstance.hpp"

bool Script::MoveFrom(Resource&& other)
{
	auto* script = dynamic_cast<Script*>(&other);
	if (script == nullptr)
	{
		return false;
	}

	m_name           = std::move(script->m_name);
	m_virtualPath    = std::move(script->m_virtualPath);
	m_sourceFilePath = std::move(script->m_sourceFilePath);
	return true;
}
