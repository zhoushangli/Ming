#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"

class Resource : public RefCounted
{
	MCLASS(Resource, RefCounted)

public:
	Resource()          = default;
	virtual ~Resource() = default;

	const std::string& GetVirtualPath() const { return m_virualPath; }
	void               SetVirtualPath(const std::string& path) { m_virualPath = path; }
	const std::string& GetName() const { return m_name; }
	void			   SetName(const std::string& name) { m_name = name; }

protected:
	static void BindMethods();

protected:
	std::string m_name;
	std::string m_virualPath;
};

class Script : public Resource
{
	MCLASS(Script, Resource)

public:
	Script()          = default;
	virtual ~Script() = default;

protected:
	static void BindMethods() {};
};