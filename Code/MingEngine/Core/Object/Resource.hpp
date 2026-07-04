#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"

#include <string>

class Resource : public RefCounted
{
	MCLASS(Resource, RefCounted)

public:
	Resource()          = default;
	virtual ~Resource() = default;

	// Actual resource path used by loaders and caches. Imported assets may point at .ming/Import files here.
	const std::string& GetVirtualPath() const { return m_virtualPath; }
	void               SetVirtualPath(const std::string& path) { m_virtualPath = path; }
	// Original user-facing source file path for editor display. Empty means this resource has no separate source file.
	const std::string& GetSourceFilePath() const { return m_sourceFilePath; }
	void               SetSourceFilePath(const std::string& path) { m_sourceFilePath = path; }
	const std::string& GetName() const { return m_name; }
	void               SetName(const std::string& name) { m_name = name; }

protected:
	static void BindMethods();

protected:
	std::string m_name;
	std::string m_virtualPath;
	std::string m_sourceFilePath;
};
