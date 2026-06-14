#pragma once

#include "MingEngine/Engine/Script/ScriptResourceIdentity.hpp"

class asIScriptModule;
class asITypeInfo;

class ScriptModule
{
	friend class ScriptSystem; // Only the ScriptSystem can create new ScriptModule objects

public:
	ScriptModule() = default;

	ScriptModule(ScriptModule const&) = delete;

	ScriptResourceIdentity const& GetIdentity() const;
	asIScriptModule*              GetScriptModule() const;
	asITypeInfo*                  GetScriptType() const;

private:
	ScriptModule(ScriptResourceIdentity identity, asIScriptModule* scriptModule, asITypeInfo* scriptType);

private:
	ScriptResourceIdentity m_identity;
	asIScriptModule*       m_scriptModule = nullptr;
	asITypeInfo*           m_scriptType   = nullptr;
};