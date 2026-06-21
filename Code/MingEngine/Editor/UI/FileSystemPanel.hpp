#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

#include <filesystem>
#include <string>
#include <vector>

class FileSystemPanel final : public EditorPanel
{
public:
	FileSystemPanel();

private:
	struct FileEntry
	{
		std::filesystem::path m_path;
		std::string           m_name;
		std::string           m_lowerName;
		std::string           m_iconName;
		bool                  m_isDirectory = false;
		std::vector<FileEntry> m_children;
	};

	void OnRender(EditorUIContext& context) override;

	void BuildFileTree(std::filesystem::path const& resourceRoot);
	FileEntry BuildEntry(std::filesystem::path const& path, bool isDirectory) const;
	void RenderEntry(FileEntry const& entry, std::string const& lowerFilterText);

	bool DoesEntryMatchFilter(FileEntry const& entry, std::string const& lowerFilterText) const;
	char const* GetIconNameForPath(std::filesystem::path const& path, bool isDirectory) const;

private:
	char      m_filter[64] = {};
	FileEntry m_rootEntry;
	bool      m_hasScanned = false;
};
