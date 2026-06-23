#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"

class Resource : public RefCounted
{
	MCLASS(Resource, RefCounted)

public:
	Resource()          = default;
	virtual ~Resource() = default;

	const std::string& GetName() const { return m_name; }
	void               SetName(const std::string& name) { m_name = name; }

	const std::string& GetPath() const { return m_path; }
	void               SetPath(const std::string& path) { m_path = path; }

protected:
	static void BindMethods();

protected:
	std::string m_name;
	std::string m_path;
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