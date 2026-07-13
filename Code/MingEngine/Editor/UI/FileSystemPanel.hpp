#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Core/Object/Object.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Editor/UI/Popup/CreateFolderPopup.hpp"
#include "MingEngine/Editor/UI/Popup/CreateScenePopup.hpp"
#include "MingEngine/Editor/UI/Popup/DeleteEntryPopup.hpp"

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
	void OpenFile(FileEntry const& entry, EditorUIContext& context);
	void BeginCreateFolder(std::string const& parentVirtualPath);
	void BeginCreateScene(std::string const& parentVirtualPath);
	void BeginRename(FileEntry const& entry);
	void FinishRename(FileSystem const& fileSystem, bool apply);
	void ClearRename();
	void BeginDelete(FileEntry const& entry);
	void DuplicateEntry(FileEntry const& entry, FileSystem& fileSystem);
	void OpenInTerminal(std::filesystem::path const& path);
	void OpenInFileManager(std::filesystem::path const& path, bool selectFile);
	void ShowError(std::string error);
	bool ValidateEntryName(char const* name, std::string& outError) const;
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
	char        m_renameBuffer[256] = {};
	bool        m_wasFocused = false;
	bool        m_focusRenameInput = false;
	bool        m_refreshResourceTree = false;
	std::string m_selectedVirtualPath;
	std::string m_renamingVirtualPath;
	std::string m_pendingRenameVirtualPath;
	std::string m_pendingRenameName;
	std::string m_directoryToOpen;
	CreateFolderPopup m_createFolderPopup;
	CreateScenePopup  m_createScenePopup;
	DeleteEntryPopup  m_deleteEntryPopup;
};
