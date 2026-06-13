#pragma once

#include <string>

struct ScriptResourceIdentity
{
	std::string m_path;
	std::string m_className;
};

bool MakeScriptResourceIdentity(std::string const& scriptPath, ScriptResourceIdentity& outIdentity);