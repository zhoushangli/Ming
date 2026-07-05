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

	// Copy content data (vertices, textures, etc.) from another resource of the same type.
	// Does NOT copy identity fields (path, name, source file path).
	// Each subclass must override this to copy its own data fields manually.
	// Returns false if other is not the same type.
	virtual bool CopyFrom(Resource const& other) = 0;

protected:
	static void BindMethods();

protected:
	std::string m_name;
	std::string m_virtualPath;
	std::string m_sourceFilePath;
};
