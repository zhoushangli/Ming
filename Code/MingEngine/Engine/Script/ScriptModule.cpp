#include "MingEngine/Engine/Script/ScriptModule.hpp"

ScriptModule::ScriptModule(ScriptResourceIdentity identity, asIScriptModule* scriptModule, asITypeInfo* scriptType)
    : m_identity(identity), m_scriptModule(scriptModule), m_scriptType(scriptType)
{
}

ScriptResourceIdentity const& ScriptModule::GetIdentity() const
{
	return m_identity;
}

asIScriptModule* ScriptModule::GetScriptModule() const
{
	return m_scriptModule;
}

asITypeInfo* ScriptModule::GetScriptType() const
{
	return m_scriptType;
}