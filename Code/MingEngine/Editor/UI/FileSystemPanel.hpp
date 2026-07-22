#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Popup/CreateFolderPopup.hpp"
#include "MingEngine/Editor/UI/Popup/CreateScenePopup.hpp"
#include "MingEngine/Editor/UI/Popup/DeleteEntryPopup.hpp"
#include "MingEngine/Engine/File/VirtualPath.hpp"

#include <filesystem>
#include <string>

class FileEntry;
class FileSystem;

class FileSystemPanel final : public EditorPanel
{
public:
	FileSystemPanel();

	VirtualPath const& GetSelectedVirtualPath() const;

private:
	void OnRender(EditorUIContext& context) override;

	void RenderEntry(FileEntry const& entry, std::string const& lowerFilterText, EditorUIContext& context);
	void RenderItemContextMenu(FileEntry const& entry, EditorUIContext& context);
	void RenderBackgroundContextMenu(EditorUIContext& context);
	void OpenFile(FileEntry const& entry, EditorUIContext& context);
	void BeginCreateFolder(VirtualPath const& parentVirtualPath);
	void BeginCreateScene(VirtualPath const& parentVirtualPath);
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
		VirtualPath const& parentVirtualPath,
		char const* name,
		std::string& outError,
		VirtualPath const& ignoredVirtualPath = {}) const;

	bool        DoesEntryMatchFilter(FileEntry const& entry, std::string const& lowerFilterText) const;
	char const* GetIconNameForPath(std::filesystem::path const& path, bool isDirectory) const;

	void MoveEntry(VirtualPath const& sourceVirtualPath, VirtualPath const& targetVirtualPath);

private:
	char        m_filter[64] = {};
	char        m_renameBuffer[256] = {};
	bool        m_wasFocused = false;
	bool        m_focusRenameInput = false;
	bool        m_refreshResourceTree = false;
	VirtualPath m_selectedVirtualPath;
	VirtualPath m_renamingVirtualPath;
	VirtualPath m_pendingRenameVirtualPath;
	std::string m_pendingRenameName;
	VirtualPath m_directoryToOpen;
	VirtualPath m_pendingMoveSourceVirtualPath;
	VirtualPath m_pendingMoveTargetVirtualPath;
	CreateFolderPopup m_createFolderPopup;
	CreateScenePopup  m_createScenePopup;
	DeleteEntryPopup  m_deleteEntryPopup;
};
