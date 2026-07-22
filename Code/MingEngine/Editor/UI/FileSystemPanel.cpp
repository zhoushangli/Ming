#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <string>

namespace
{
std::string ToLower(std::string text)
{
	std::transform(
		text.begin(),
		text.end(),
		text.begin(),
		[](unsigned char character) { return static_cast<char>(std::tolower(character)); });
	return text;
}

bool IsReservedWindowsName(std::string const& name)
{
	std::string const stem = ToLower(std::filesystem::path(name).stem().string());
	if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul")
	{
		return true;
	}
	return stem.size() == 4 && (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0) && stem[3] >= '1'
		   && stem[3] <= '9';
}

FileEntry const* FindEntry(FileEntry const& entry, VirtualPath const& virtualPath)
{
	if (entry.GetVirtualPath() == virtualPath)
	{
		return &entry;
	}
	for (std::unique_ptr<FileEntry> const& child : entry.GetChildren())
	{
		if (FileEntry const* found = FindEntry(*child, virtualPath))
		{
			return found;
		}
	}
	return nullptr;
}

} // namespace

FileSystemPanel::FileSystemPanel() : EditorPanel("FileSystem") {}

VirtualPath const& FileSystemPanel::GetSelectedVirtualPath() const { return m_selectedVirtualPath; }

void FileSystemPanel::OnRender(EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());
	bool const isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	ImGui::InputTextWithHint("##FilterFiles", "Filter Files", m_filter, sizeof(m_filter));
	ImGui::SameLine();
	ImGui::BeginDisabled(!m_selectedVirtualPath.IsValid());
	bool const openSelected = ImGui::Button("Open");
	ImGui::EndDisabled();
	ImGui::Separator();

	if (context.m_fileSystem == nullptr)
	{
		ImGui::TextDisabled("FileSystem is not available.");
		m_wasFocused = isFocused;
		ImGui::End();
		return;
	}

	if (!context.m_fileSystem->HasResourceTree() || (isFocused && !m_wasFocused))
	{
		context.m_fileSystem->ScanResourceTree();
	}

	FileEntry const* rootEntry = context.m_fileSystem->GetResourceRootEntry();
	if (rootEntry == nullptr)
	{
		ImGui::TextUnformatted("res://");
		ImGui::TextDisabled("Resource root not found.");
		m_wasFocused = isFocused;
		ImGui::End();
		return;
	}
	if (openSelected)
	{
		if (FileEntry const* selectedEntry = FindEntry(*rootEntry, m_selectedVirtualPath))
		{
			OpenFile(*selectedEntry, context);
		}
	}

	ImGui::BeginChild("##FileSystemTreeScrollRegion", ImVec2(0.f, 0.f), false);

	std::string const lowerFilterText = ToLower(m_filter);
	RenderEntry(*rootEntry, lowerFilterText, context);
	if (isFocused && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete)
		&& m_selectedVirtualPath.IsValid() && !m_selectedVirtualPath.IsRoot())
	{
		if (FileEntry const* selectedEntry = FindEntry(*rootEntry, m_selectedVirtualPath))
		{
			BeginDelete(*selectedEntry);
		}
	}
	if (isFocused && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_F2) && !m_renamingVirtualPath.IsValid()
		&& m_selectedVirtualPath.IsValid() && !m_selectedVirtualPath.IsRoot())
	{
		if (FileEntry const* selectedEntry = FindEntry(*rootEntry, m_selectedVirtualPath))
		{
			BeginRename(*selectedEntry);
		}
	}
	RenderBackgroundContextMenu(context);

	ImGui::EndChild();

	// Handle pending operations that need to be executed after the UI rendering.
	if (m_pendingRenameVirtualPath.IsValid())
	{
		VirtualPath newVirtualPath;
		std::string error;
		if (context.m_fileSystem->Rename(m_pendingRenameVirtualPath, m_pendingRenameName, newVirtualPath, error))
		{
			context.m_fileSystem->ScanResourceTree();
			m_selectedVirtualPath = newVirtualPath;
		}
		else
		{
			ShowError(std::move(error));
		}
		m_pendingRenameVirtualPath = {};
		m_pendingRenameName.clear();
	}

	if (m_pendingMoveSourceVirtualPath.IsValid() && m_pendingMoveTargetVirtualPath.IsValid())
	{
		VirtualPath newVirtualPath;
		std::string error;
		if (context.m_fileSystem->Move(
				m_pendingMoveSourceVirtualPath, m_pendingMoveTargetVirtualPath, newVirtualPath, error))
		{
			context.m_fileSystem->ScanResourceTree();
			m_selectedVirtualPath = newVirtualPath;
		}
		else
		{
			ShowError(std::move(error));
		}
		m_pendingMoveSourceVirtualPath = {};
		m_pendingMoveTargetVirtualPath = {};
	}

	if (m_refreshResourceTree)
	{
		context.m_fileSystem->ScanResourceTree();
		m_refreshResourceTree = false;
	}

	m_wasFocused = isFocused;
	ImGui::End();

	m_createFolderPopup.Render(context);
	m_createScenePopup.Render(context);
	m_deleteEntryPopup.Render(context);
	m_createFolderPopup.ConsumeSelectedVirtualPath(m_selectedVirtualPath);
	m_createScenePopup.ConsumeSelectedVirtualPath(m_selectedVirtualPath);
	m_deleteEntryPopup.ConsumeSelectedVirtualPath(m_selectedVirtualPath);
}

void FileSystemPanel::RenderEntry(FileEntry const& entry, std::string const& lowerFilterText, EditorUIContext& context)
{
	bool const hasVisibleChildren = std::any_of(
		entry.GetChildren().begin(),
		entry.GetChildren().end(),
		[this, &lowerFilterText](std::unique_ptr<FileEntry> const& child)
		{ return DoesEntryMatchFilter(*child, lowerFilterText); });

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (entry.GetParent() == nullptr)
	{
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	if (!entry.IsDirectory() || !hasVisibleChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (!lowerFilterText.empty())
	{
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}
	if (m_selectedVirtualPath == entry.GetVirtualPath())
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	ImGui::PushID(entry.GetVirtualPath().CStr());
	if (m_directoryToOpen == entry.GetVirtualPath())
	{
		ImGui::SetNextItemOpen(true, ImGuiCond_Always);
		m_directoryToOpen = {};
	}
	bool const   isOpen = ImGui::TreeNodeEx(entry.IsDirectory() ? "##Directory" : "##File", flags);
	ImVec2 const rowMin = ImGui::GetItemRectMin();
	ImVec2 const rowMax = ImGui::GetItemRectMax();
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		m_selectedVirtualPath = entry.GetVirtualPath();
	}
	if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
	{
		OpenFile(entry, context);
	}
	RenderItemContextMenu(entry, context);
	bool const isRenaming = m_renamingVirtualPath == entry.GetVirtualPath();

	if (isRenaming)
	{
		ImVec2 const iconSize = EditorUIStyle::FileTreeIconSize();
		ImGui::SameLine();
		float const iconY = rowMin.y + (rowMax.y - rowMin.y - iconSize.y) * 0.5f;
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, iconY));
		EditorUIWidgets::RenderIcon(
			GetIconNameForPath(entry.GetPhysicalPath(), entry.IsDirectory()),
			entry.IsDirectory() ? "Folder" : "File",
			iconSize);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		if (m_focusRenameInput)
		{
			ImGui::SetKeyboardFocusHere();
			m_focusRenameInput = false;
		}
		bool const submitted = ImGui::InputText(
			"##FileName",
			m_renameBuffer,
			sizeof(m_renameBuffer),
			ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
		bool const cancelled   = ImGui::IsItemActive() && ImGui::IsKeyPressed(ImGuiKey_Escape);
		bool const deactivated = ImGui::IsItemDeactivated();
		if (cancelled)
		{
			FinishRename(*context.m_fileSystem, false);
		}
		else if (submitted || deactivated)
		{
			FinishRename(*context.m_fileSystem, true);
		}
	}
	else
	{
		if (ImGui::BeginDragDropSource())
		{
			// Submit a marker payload so ImGui keeps the drag source active.
			// e.g. The actual virtual path remains in EditorDragDrop.
			EditorNode::Get()->m_dragDrop.SetDragData(entry.GetVirtualPath());
			ImGui::SetDragDropPayload(EditorDragDrop::PayloadType, nullptr, 0);
			ImGui::TextUnformatted(entry.GetName().c_str());

			ImGui::EndDragDropSource();
		}

		if (entry.IsDirectory() && ImGui::BeginDragDropTarget())
		{
			EditorDragDrop& dragDrop = EditorNode::Get()->m_dragDrop;
			VirtualPath     draggedVirtualPath;
			bool const      canMove = dragDrop.TryGetData(draggedVirtualPath) && !draggedVirtualPath.IsRoot()
				&& draggedVirtualPath != entry.GetVirtualPath()
				&& draggedVirtualPath.GetParent() != entry.GetVirtualPath();
			if (canMove)
			{
				dragDrop.AllowDrop();
				if (ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadType) != nullptr)
				{
					MoveEntry(draggedVirtualPath, entry.GetVirtualPath());
				}
			}
			ImGui::EndDragDropTarget();
		}

		EditorUIWidgets::RenderTreeRowContent(
			GetIconNameForPath(entry.GetPhysicalPath(), entry.IsDirectory()),
			entry.IsDirectory() ? "Folder" : "File",
			entry.GetName(),
			rowMin,
			rowMax,
			EditorUIStyle::FileTreeIconSize());
	}

	if (isOpen && entry.IsDirectory() && hasVisibleChildren)
	{
		for (std::unique_ptr<FileEntry> const& child : entry.GetChildren())
		{
			if (!DoesEntryMatchFilter(*child, lowerFilterText))
			{
				continue;
			}

			RenderEntry(*child, lowerFilterText, context);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

void FileSystemPanel::RenderItemContextMenu(FileEntry const& entry, EditorUIContext& context)
{
	if (!ImGui::BeginPopupContextItem("FileSystemItemContext"))
	{
		return;
	}

	m_selectedVirtualPath        = entry.GetVirtualPath();
	bool const        isRoot     = entry.GetParent() == nullptr;
	VirtualPath const createBase = entry.IsDirectory() ? entry.GetVirtualPath() : entry.GetVirtualPath().GetParent();
	if (ImGui::MenuItem("Open"))
	{
		OpenFile(entry, context);
	}
	ImGui::Separator();
	if (ImGui::BeginMenu("Create New"))
	{
		if (ImGui::MenuItem("Folder..."))
		{
			BeginCreateFolder(createBase);
		}
		if (ImGui::MenuItem("Scene..."))
		{
			BeginCreateScene(createBase);
		}
		ImGui::EndMenu();
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Copy Path"))
	{
		ImGui::SetClipboardText(entry.GetVirtualPath().CStr());
	}
	if (ImGui::MenuItem("Copy Absolute Path"))
	{
		ImGui::SetClipboardText(entry.GetPhysicalPath().lexically_normal().string().c_str());
	}
	if (ImGui::MenuItem("Rename...", nullptr, false, !isRoot))
	{
		BeginRename(entry);
	}
	if (ImGui::MenuItem("Duplicate...", nullptr, false, !isRoot) && context.m_fileSystem != nullptr)
	{
		DuplicateEntry(entry, *context.m_fileSystem);
	}
	if (ImGui::MenuItem("Delete", nullptr, false, !isRoot))
	{
		BeginDelete(entry);
	}
	ImGui::Separator();
	if (entry.IsDirectory() && ImGui::MenuItem("Open in Terminal"))
	{
		OpenInTerminal(entry.GetPhysicalPath());
	}
	if (ImGui::MenuItem("Open in File Manager"))
	{
		OpenInFileManager(entry.GetPhysicalPath(), !entry.IsDirectory());
	}
	ImGui::EndPopup();
}

void FileSystemPanel::RenderBackgroundContextMenu(EditorUIContext& context)
{
	if (!ImGui::BeginPopupContextWindow(
			"FileSystemBackgroundContext",
			ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		return;
	}

	if (ImGui::MenuItem("New Folder..."))
	{
		BeginCreateFolder("res://");
	}
	if (ImGui::MenuItem("New Scene..."))
	{
		BeginCreateScene("res://");
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Open in Terminal"))
	{
		OpenInTerminal(context.m_fileSystem->GetResourceRoot());
	}
	if (ImGui::MenuItem("Open in File Manager"))
	{
		OpenInFileManager(context.m_fileSystem->GetResourceRoot(), false);
	}
	ImGui::EndPopup();
}

void FileSystemPanel::BeginCreateFolder(VirtualPath const& parentVirtualPath)
{
	m_createFolderPopup.Open(parentVirtualPath);
}

void FileSystemPanel::OpenFile(FileEntry const& entry, [[maybe_unused]] EditorUIContext& context)
{
	if (entry.IsDirectory())
	{
		m_directoryToOpen = entry.GetVirtualPath();
		return;
	}

	std::string extension = ToLower(entry.GetPhysicalPath().extension().string());
	if (extension == ".tscn" && EditorNode::Get() != nullptr)
	{
		EditorNode::Get()->RequestLoadScene(entry.GetVirtualPath());
	}
}

void FileSystemPanel::BeginCreateScene(VirtualPath const& parentVirtualPath)
{
	m_createScenePopup.Open(parentVirtualPath);
}

void FileSystemPanel::BeginRename(FileEntry const& entry)
{
	m_selectedVirtualPath = entry.GetVirtualPath();
	m_renamingVirtualPath = entry.GetVirtualPath();
	strncpy_s(m_renameBuffer, entry.GetName().c_str(), sizeof(m_renameBuffer) - 1);
	m_focusRenameInput = true;
}

void FileSystemPanel::FinishRename(FileSystem const& fileSystem, bool apply)
{
	if (!apply)
	{
		ClearRename();
		return;
	}

	VirtualPath const parentVirtualPath = m_renamingVirtualPath.GetParent();
	if (parentVirtualPath.Join(m_renameBuffer) == m_renamingVirtualPath)
	{
		ClearRename();
		return;
	}

	std::string error;
	if (!ValidateName(fileSystem, parentVirtualPath, m_renameBuffer, error, m_renamingVirtualPath))
	{
		ShowError(std::move(error));
		ClearRename();
		return;
	}

	m_pendingRenameVirtualPath = m_renamingVirtualPath;
	m_pendingRenameName        = m_renameBuffer;
	ClearRename();
}

void FileSystemPanel::ClearRename()
{
	m_renamingVirtualPath = {};
	m_renameBuffer[0]     = '\0';
	m_focusRenameInput    = false;
}

void FileSystemPanel::BeginDelete(FileEntry const& entry)
{
	m_deleteEntryPopup.Open(entry.GetVirtualPath(), entry.IsDirectory());
}

void FileSystemPanel::DuplicateEntry(FileEntry const& entry, FileSystem& fileSystem)
{
	VirtualPath duplicateVirtualPath;
	std::string error;
	if (!fileSystem.Duplicate(entry.GetVirtualPath(), duplicateVirtualPath, error))
	{
		ShowError(std::move(error));
		return;
	}
	m_selectedVirtualPath = duplicateVirtualPath;
	m_refreshResourceTree = true;
}

void FileSystemPanel::OpenInTerminal(std::filesystem::path const& path)
{
	std::wstring const directory        = std::filesystem::absolute(path).wstring();
	std::wstring       escapedDirectory = directory;
	size_t             position         = 0;
	while ((position = escapedDirectory.find(L'\'', position)) != std::wstring::npos)
	{
		escapedDirectory.insert(position, 1, L'\'');
		position += 2;
	}
	std::wstring const parameters = L"-NoExit -Command Set-Location -LiteralPath '" + escapedDirectory + L"'";
	HINSTANCE const    result =
		ShellExecuteW(nullptr, L"open", L"powershell.exe", parameters.c_str(), directory.c_str(), SW_SHOWNORMAL);
	if (reinterpret_cast<intptr_t>(result) <= 32)
	{
		ShowError("Failed to open terminal.");
	}
}

void FileSystemPanel::OpenInFileManager(std::filesystem::path const& path, bool selectFile)
{
	std::wstring const absolutePath = std::filesystem::absolute(path).wstring();
	std::wstring const parameters   = selectFile ? L"/select,\"" + absolutePath + L"\"" : L"\"" + absolutePath + L"\"";
	HINSTANCE const    result =
		ShellExecuteW(nullptr, L"open", L"explorer.exe", parameters.c_str(), nullptr, SW_SHOWNORMAL);
	if (reinterpret_cast<intptr_t>(result) <= 32)
	{
		ShowError("Failed to open File Manager.");
	}
}

void FileSystemPanel::ShowError(std::string error)
{
	if (EditorNode::Get() != nullptr && EditorNode::Get()->m_editorUI != nullptr)
	{
		EditorNode::Get()->m_editorUI->Warning("FileSystem Error", error);
	}
}

bool FileSystemPanel::ValidateEntryName(char const* name, std::string& outError) const
{
	std::string const value = name != nullptr ? name : "";
	if (value.empty())
	{
		outError = "Name cannot be empty.";
		return false;
	}
	if (value == "." || value == ".." || value.front() == ' ' || value.back() == ' ' || value.back() == '.')
	{
		outError = "Name cannot be '.', '..', or start/end with a space.";
		return false;
	}
	if (value.find_first_of("<>:\"/\\|?*") != std::string::npos || IsReservedWindowsName(value))
	{
		outError = "Name contains invalid characters or is reserved by Windows.";
		return false;
	}
	outError.clear();
	return true;
}

bool FileSystemPanel::ValidateName(
	FileSystem const&  fileSystem,
	VirtualPath const& parentVirtualPath,
	char const*        name,
	std::string&       outError,
	VirtualPath const& ignoredVirtualPath) const
{
	std::string const value = name != nullptr ? name : "";
	if (!ValidateEntryName(name, outError))
	{
		return false;
	}
	VirtualPath const candidate = parentVirtualPath.Join(value);
	if (candidate == ignoredVirtualPath)
	{
		outError = "Name has not changed.";
		return false;
	}
	if (fileSystem.Exists(candidate))
	{
		outError = "An entry with this name already exists.";
		return false;
	}
	outError.clear();
	return true;
}

bool FileSystemPanel::DoesEntryMatchFilter(FileEntry const& entry, std::string const& lowerFilterText) const
{
	if (lowerFilterText.empty())
	{
		return true;
	}

	if (entry.GetLowerName().find(lowerFilterText) != std::string::npos)
	{
		return true;
	}

	for (std::unique_ptr<FileEntry> const& child : entry.GetChildren())
	{
		if (DoesEntryMatchFilter(*child, lowerFilterText))
		{
			return true;
		}
	}

	return false;
}

char const* FileSystemPanel::GetIconNameForPath(std::filesystem::path const& path, bool isDirectory) const
{
	if (isDirectory)
	{
		return "Folder";
	}

	std::string const extension = ToLower(path.extension().string());
	if (extension == ".tscn" || extension == ".json")
	{
		return "PackedScene";
	}
	if (extension == ".cs" || extension == ".as" || extension == ".gd")
	{
		return "Script";
	}
	if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".svg")
	{
		return "Texture2D";
	}
	if (extension == ".fbx" || extension == ".obj" || extension == ".glb" || extension == ".gltf")
	{
		return "Mesh";
	}
	if (extension == ".wav" || extension == ".mp3" || extension == ".ogg")
	{
		return "AudioStream";
	}
	if (extension == ".shader" || extension == ".hlsl")
	{
		return "Shader";
	}

	return "File";
}

void FileSystemPanel::MoveEntry(VirtualPath const& sourceVirtualPath, VirtualPath const& targetVirtualPath)
{
	m_pendingMoveSourceVirtualPath = sourceVirtualPath;
	m_pendingMoveTargetVirtualPath = targetVirtualPath;
}
