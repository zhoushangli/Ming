#pragma once

#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <string>

struct ScriptResourceIdentity
{
public:
	ScriptResourceIdentity() = default;

	static bool Create(VirtualPath const& path, ScriptResourceIdentity& outIdentity);

	VirtualPath const&  GetPath() const;
	std::string const& GetClassName() const;

private:
	VirtualPath m_path;
	std::string m_className;
};