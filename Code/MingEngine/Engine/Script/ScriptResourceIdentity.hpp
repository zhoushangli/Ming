#pragma once

#include <string>

struct ScriptResourceIdentity
{
public:
	ScriptResourceIdentity() = default;

	static bool Create(std::string const& virtualPath, ScriptResourceIdentity& outIdentity);

	std::string const& GetVirtualPath() const;
	std::string const& GetClassName() const;

private:
	std::string m_virtualPath;
	std::string m_className;
};
