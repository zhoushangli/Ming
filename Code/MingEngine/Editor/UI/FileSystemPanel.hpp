#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"

#include <filesystem>
#include <string>

struct FilePayload
{
	Ref<Resource> m_resource;
};

class FileEntry;

class FileSystemPanel final : public EditorPanel
{
public:
	FileSystemPanel();

	std::string const& GetSelectedVirtualPath() const;

private:
	void OnRender(EditorUIContext& context) override;

	void RenderEntry(FileEntry const& entry, std::string const& lowerFilterText);

	bool        DoesEntryMatchFilter(FileEntry const& entry, std::string const& lowerFilterText) const;
	char const* GetIconNameForPath(std::filesystem::path const& path, bool isDirectory) const;

private:
	char        m_filter[64] = {};
	bool        m_wasFocused = false;
	std::string m_selectedVirtualPath;
};
