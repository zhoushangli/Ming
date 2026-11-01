#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Editor/UI/Popup/CreateFolderPopup.hpp"
#include "MingEngine/Editor/UI/Popup/CreateScenePopup.hpp"
#include "MingEngine/Editor/UI/Popup/DeleteEntryPopup.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"

#include <filesystem>
#include <map>

class FileEntry;
class FileSystem;

class FileSystemPanel final : public EditorPanel
{
	MCLASS(FileSystemPanel, EditorPanel);

public:
	FileSystemPanel();
	VirtualPath const& GetSelectedVirtualPath() const { return m_selectedVirtualPath; }
	Tree* GetTree() const { return m_tree; }

protected:
	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;

private:
	void UpdateItems();
	FileEntry const* GetSelectedEntry() const;
	void OpenSelected();
	void ShowContextMenu(Vector2 position);
	void DeleteSelected();
	bool HandleTreeInput(InputEvent const& event);
	bool CanMove(VirtualPath const& source, uint64_t target) const;
	void OpenInTerminal(std::filesystem::path const& path);
	void OpenInFileManager(std::filesystem::path const& path, bool selectFile);
	void ShowError(std::string error);
	bool ValidateEntryName(char const* name, std::string& outError) const;
	bool ValidateName(FileSystem const& fileSystem, VirtualPath const& parent, char const* name,
		std::string& error, VirtualPath const& ignored = {}) const;

private:
	Tree* m_tree = nullptr;
	LineEdit* m_filter = nullptr;
	VirtualPath m_selectedVirtualPath;
	std::map<uint64_t, VirtualPath> m_fileItems;
	std::map<std::string, uint64_t> m_fileKeys;
	uint64_t m_nextFileKey = 1;
	CreateFolderPopup m_createFolderPopup;
	CreateScenePopup m_createScenePopup;
	DeleteEntryPopup m_deleteEntryPopup;
};
