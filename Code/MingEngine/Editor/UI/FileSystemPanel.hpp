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
class FileSystem;

class FileSystemPanel final : public EditorPanel
{
public:
	FileSystemPanel();

	std::string const& GetSelectedVirtualPath() const;

private:
	void OnRender(EditorUIContext& context) override;

	void RenderEntry(FileEntry const& entry, std::string const& lowerFilterText, EditorUIContext& context);
	void RenderItemContextMenu(FileEntry const& entry, EditorUIContext& context);
	void RenderBackgroundContextMenu(EditorUIContext& context);
	void RenderCreateFolderPopup(EditorUIContext& context);
	void RenderRenamePopup(EditorUIContext& context);
	void RenderDeletePopup(EditorUIContext& context);
	void RenderErrorPopup();

	void BeginCreateFolder(std::string const& parentVirtualPath);
	void BeginRename(FileEntry const& entry);
	void BeginDelete(FileEntry const& entry);
	void DuplicateEntry(FileEntry const& entry, FileSystem& fileSystem);
	void OpenInTerminal(std::filesystem::path const& path);
	void OpenInFileManager(std::filesystem::path const& path, bool selectFile);
	void ShowError(std::string error);
	bool ValidateName(
		FileSystem const& fileSystem,
		std::string const& parentVirtualPath,
		char const* name,
		std::string& outError,
		std::string const& ignoredVirtualPath = {}) const;

	bool        DoesEntryMatchFilter(FileEntry const& entry, std::string const& lowerFilterText) const;
	char const* GetIconNameForPath(std::filesystem::path const& path, bool isDirectory) const;

private:
	char        m_filter[64] = {};
	char        m_nameBuffer[256] = {};
	bool        m_wasFocused = false;
	bool        m_focusNameInput = false;
	bool        m_openCreateFolderPopup = false;
	bool        m_openRenamePopup = false;
	bool        m_openDeletePopup = false;
	bool        m_openErrorPopup = false;
	bool        m_operationTargetIsDirectory = false;
	bool        m_refreshResourceTree = false;
	std::string m_selectedVirtualPath;
	std::string m_operationBaseVirtualPath;
	std::string m_operationTargetVirtualPath;
	std::string m_operationError;
};
