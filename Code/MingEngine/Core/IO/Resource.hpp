#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/String.hpp"
#include "MingEngine/Core/IO/VirtualPath.hpp"

class Resource : public RefCounted
{
	MCLASS(Resource, RefCounted)

public:
	Resource()          = default;
	virtual ~Resource() = default;

	// Actual resource path used by loaders and caches. Imported assets may point at .ming/Import files here.
	VirtualPath const& GetVirtualPath() const { return m_virtualPath; }

	void SetVirtualPath(VirtualPath const& path) { m_virtualPath = path; }

	// Original user-facing source file path for editor display. Empty means this resource has no separate source file.
	VirtualPath const& GetSourceFilePath() const { return m_sourceFilePath; }

	void SetSourceFilePath(VirtualPath const& path) { m_sourceFilePath = path; }

	String const& GetName() const { return m_name; }

	void SetName(String const& name) { m_name = name; }

	// Receive data from another resource of the same type while preserving this resource instance.
	// e.g. cachedResource.MoveFrom(std::move(*freshResource)) reloads the cached resource in place.
	virtual bool MoveFrom(Resource&& other) = 0;

protected:
	static void BindMethods();
	String      GetPathString() const;
	void        SetPathString(String const& path);

protected:
	String      m_name;
	VirtualPath m_virtualPath;
	VirtualPath m_sourceFilePath;
};
