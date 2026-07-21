#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <string>

class Resource : public RefCounted
{
	MCLASS(Resource, RefCounted)

public:
	Resource()          = default;
	virtual ~Resource() = default;

	// Actual resource path used by loaders and caches. Imported assets may point at .ming/Import files here.
	VirtualPath const& GetVirtualPath() const { return m_virtualPath; }
	void               SetVirtualPath(VirtualPath const& path) { m_virtualPath = path; }
	// Original user-facing source file path for editor display. Empty means this resource has no separate source file.
	VirtualPath const& GetSourceFilePath() const { return m_sourceFilePath; }
	void               SetSourceFilePath(VirtualPath const& path) { m_sourceFilePath = path; }
	const std::string& GetName() const { return m_name; }
	void               SetName(const std::string& name) { m_name = name; }

	// Move all data from another resource of the same type and leave the source valid but empty.
	// e.g. cachedResource.MoveFrom(std::move(freshResource))
	virtual bool MoveFrom(Resource&& other) = 0;

protected:
	static void BindMethods();
	void        MoveBaseFrom(Resource&& other);
	std::string GetPathString() const;
	void        SetPathString(std::string const& path);

protected:
	std::string m_name;
	VirtualPath m_virtualPath;
	VirtualPath m_sourceFilePath;
};
