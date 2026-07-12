#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <Windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cctype>
#include <cfloat>
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

std::string GetParentVirtualPath(std::string const& virtualPath)
{
	size_t const separator = virtualPath.find_last_of('/');
	return separator <= 5 ? "res://" : virtualPath.substr(0, separator);
}

std::string JoinVirtualPath(std::string const& parentVirtualPath, std::string const& name)
{
	return parentVirtualPath == "res://" ? parentVirtualPath + name : parentVirtualPath + "/" + name;
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

FileEntry const* FindEntry(FileEntry const& entry, std::string const& virtualPath)
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

bool BeginFileSystemModal(char const* title)
{
	ImGui::SetNextWindowSizeConstraints(ImVec2(520.f, 0.f), ImVec2(FLT_MAX, FLT_MAX));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	bool const isOpen = ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::PopStyleColor();
	return isOpen;
}
} // namespace

FileSystemPanel::FileSystemPanel() : EditorPanel("FileSystem") {}

std::string const& FileSystemPanel::GetSelectedVirtualPath() const { return m_selectedVirtualPath; }

void FileSystemPanel::OnRender(EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());
	bool const isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	ImGui::InputTextWithHint("##FilterFiles", "Filter Files", m_filter, sizeof(m_filter));
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

	ImGui::BeginChild("##FileSystemTreeScrollRegion", ImVec2(0.f, 0.f), false);

	std::string const lowerFilterText = ToLower(m_filter);
	RenderEntry(*rootEntry, lowerFilterText, context);
	if (isFocused && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete) && !m_selectedVirtualPath.empty()
		&& m_selectedVirtualPath != "res://")
	{
		if (FileEntry const* selectedEntry = FindEntry(*rootEntry, m_selectedVirtualPath))
		{
			BeginDelete(*selectedEntry);
		}
	}
	RenderBackgroundContextMenu(context);

	ImGui::EndChild();
	if (m_refreshResourceTree)
	{
		context.m_fileSystem->ScanResourceTree();
		m_refreshResourceTree = false;
	}
	RenderCreateFolderPopup(context);
	RenderRenamePopup(context);
	RenderDeletePopup(context);
	RenderErrorPopup();
	m_wasFocused = isFocused;
	ImGui::End();
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

	ImGui::PushID(entry.GetVirtualPath().c_str());
	bool const   isOpen = ImGui::TreeNodeEx(entry.IsDirectory() ? "##Directory" : "##File", flags);
	ImVec2 const rowMin = ImGui::GetItemRectMin();
	ImVec2 const rowMax = ImGui::GetItemRectMax();
	if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
	{
		m_selectedVirtualPath = entry.GetVirtualPath();
	}
	RenderItemContextMenu(entry, context);

	if (!entry.IsDirectory())
	{
		if (ImGui::BeginDragDropSource())
		{
			FilePayload payload;

			payload.m_resource = ResourceLoader::Load(entry.GetVirtualPath());
			if (payload.m_resource.IsValid())
			{
				ImGui::SetDragDropPayload("FILESYSTEM_RESOURCE", &payload, sizeof(payload));
				ImGui::TextUnformatted(entry.GetName().c_str());
			}

			ImGui::EndDragDropSource();
		}
	}

	EditorUIWidgets::RenderTreeRowContent(
		GetIconNameForPath(entry.GetPhysicalPath(), entry.IsDirectory()),
		entry.IsDirectory() ? "Folder" : "File",
		entry.GetName(),
		rowMin,
		rowMax,
		EditorUIStyle::FileTreeIconSize());

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

	m_selectedVirtualPath    = entry.GetVirtualPath();
	bool const        isRoot = entry.GetParent() == nullptr;
	std::string const createBase =
		entry.IsDirectory() ? entry.GetVirtualPath() : GetParentVirtualPath(entry.GetVirtualPath());
	if (ImGui::BeginMenu("Create New"))
	{
		if (ImGui::MenuItem("Folder..."))
		{
			BeginCreateFolder(createBase);
		}
		ImGui::EndMenu();
	}
	ImGui::Separator();
	if (ImGui::MenuItem("Copy Path"))
	{
		ImGui::SetClipboardText(entry.GetVirtualPath().c_str());
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

void FileSystemPanel::RenderCreateFolderPopup(EditorUIContext& context)
{
	if (m_openCreateFolderPopup)
	{
		ImGui::OpenPopup("Create Folder");
		m_openCreateFolderPopup = false;
	}
	if (!BeginFileSystemModal("Create Folder"))
	{
		return;
	}

	ImGui::Text("Base path: %s", m_operationBaseVirtualPath.c_str());
	ImGui::TextUnformatted("Name:");
	if (m_focusNameInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusNameInput = false;
	}
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool const submitted = ImGui::InputText(
		"##CreateFolderName",
		m_nameBuffer,
		sizeof(m_nameBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

	std::string validationError;
	bool const isValid = ValidateName(*context.m_fileSystem, m_operationBaseVirtualPath, m_nameBuffer, validationError);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("##CreateFolderValidation", ImVec2(0.f, 42.f), true);
	ImGui::TextColored(
		isValid ? ImVec4(0.25f, 0.85f, 0.35f, 1.f) : ImVec4(0.9f, 0.35f, 0.3f, 1.f),
		"%s",
		isValid ? "Folder name is valid." : validationError.c_str());
	ImGui::EndChild();
	ImGui::PopStyleColor();

	float const buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	bool const  create      = (ImGui::Button("Create", ImVec2(buttonWidth, 0.f)) || submitted) && isValid;
	ImGui::SameLine();
	bool const cancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0.f)) || ImGui::IsKeyPressed(ImGuiKey_Escape);
	if (create)
	{
		std::string newVirtualPath;
		if (context.m_fileSystem
				->CreateFolder(m_operationBaseVirtualPath, m_nameBuffer, newVirtualPath, m_operationError))
		{
			context.m_fileSystem->ScanResourceTree();
			m_selectedVirtualPath = newVirtualPath;
			ImGui::CloseCurrentPopup();
		}
	}
	if (!m_operationError.empty())
	{
		ImGui::TextColored(ImVec4(0.9f, 0.35f, 0.3f, 1.f), "%s", m_operationError.c_str());
	}
	if (cancel)
	{
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void FileSystemPanel::RenderRenamePopup(EditorUIContext& context)
{
	if (m_openRenamePopup)
	{
		ImGui::OpenPopup("Rename Entry");
		m_openRenamePopup = false;
	}
	if (!BeginFileSystemModal("Rename Entry"))
	{
		return;
	}

	ImGui::Text("Path: %s", m_operationTargetVirtualPath.c_str());
	ImGui::TextUnformatted("Name:");
	if (m_focusNameInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusNameInput = false;
	}
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool const submitted = ImGui::InputText(
		"##RenameName",
		m_nameBuffer,
		sizeof(m_nameBuffer),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
	std::string validationError;
	bool const  isValid = ValidateName(
		*context.m_fileSystem,
		m_operationBaseVirtualPath,
		m_nameBuffer,
		validationError,
		m_operationTargetVirtualPath);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("##RenameValidation", ImVec2(0.f, 42.f), true);
	ImGui::TextColored(
		isValid ? ImVec4(0.25f, 0.85f, 0.35f, 1.f) : ImVec4(0.9f, 0.35f, 0.3f, 1.f),
		"%s",
		isValid ? "Name is valid." : validationError.c_str());
	ImGui::EndChild();
	ImGui::PopStyleColor();

	float const buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	bool const  rename      = (ImGui::Button("Rename", ImVec2(buttonWidth, 0.f)) || submitted) && isValid;
	ImGui::SameLine();
	bool const cancel = ImGui::Button("Cancel", ImVec2(buttonWidth, 0.f)) || ImGui::IsKeyPressed(ImGuiKey_Escape);
	if (rename)
	{
		std::string newVirtualPath;
		if (context.m_fileSystem->Rename(m_operationTargetVirtualPath, m_nameBuffer, newVirtualPath, m_operationError))
		{
			context.m_fileSystem->ScanResourceTree();
			m_selectedVirtualPath = newVirtualPath;
			ImGui::CloseCurrentPopup();
		}
	}
	if (!m_operationError.empty())
	{
		ImGui::TextColored(ImVec4(0.9f, 0.35f, 0.3f, 1.f), "%s", m_operationError.c_str());
	}
	if (cancel)
	{
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void FileSystemPanel::RenderDeletePopup(EditorUIContext& context)
{
	if (m_openDeletePopup)
	{
		ImGui::OpenPopup("Delete Entry");
		m_openDeletePopup = false;
	}
	if (!BeginFileSystemModal("Delete Entry"))
	{
		return;
	}

	ImGui::Text("Delete %s?", m_operationTargetVirtualPath.c_str());
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("##DeleteWarning", ImVec2(0.f, 76.f), true);
	if (m_operationTargetIsDirectory)
	{
		ImGui::TextWrapped("The folder and all of its contents will be permanently deleted.");
	}
	else
	{
		ImGui::TextWrapped("The file will be permanently deleted.");
	}
	ImGui::TextColored(ImVec4(0.9f, 0.55f, 0.2f, 1.f), "This action does not use the Recycle Bin.");
	ImGui::EndChild();
	ImGui::PopStyleColor();
	float const buttonWidth = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	if (ImGui::Button("Delete", ImVec2(buttonWidth, 0.f)))
	{
		if (context.m_fileSystem->Remove(m_operationTargetVirtualPath, m_operationError))
		{
			context.m_fileSystem->ScanResourceTree();
			m_selectedVirtualPath = m_operationBaseVirtualPath;
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0.f)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		ImGui::CloseCurrentPopup();
	}
	if (!m_operationError.empty())
	{
		ImGui::TextColored(ImVec4(0.9f, 0.35f, 0.3f, 1.f), "%s", m_operationError.c_str());
	}
	ImGui::EndPopup();
}

void FileSystemPanel::RenderErrorPopup()
{
	if (m_openErrorPopup)
	{
		ImGui::OpenPopup("FileSystem Error");
		m_openErrorPopup = false;
	}
	if (BeginFileSystemModal("FileSystem Error"))
	{
		ImGui::TextWrapped("%s", m_operationError.c_str());
		if (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void FileSystemPanel::BeginCreateFolder(std::string const& parentVirtualPath)
{
	m_operationBaseVirtualPath = parentVirtualPath;
	m_operationTargetVirtualPath.clear();
	m_operationError.clear();
	strcpy_s(m_nameBuffer, "New Folder");
	m_focusNameInput        = true;
	m_openCreateFolderPopup = true;
}

void FileSystemPanel::BeginRename(FileEntry const& entry)
{
	m_operationBaseVirtualPath   = GetParentVirtualPath(entry.GetVirtualPath());
	m_operationTargetVirtualPath = entry.GetVirtualPath();
	m_operationError.clear();
	strncpy_s(m_nameBuffer, entry.GetName().c_str(), sizeof(m_nameBuffer) - 1);
	m_focusNameInput  = true;
	m_openRenamePopup = true;
}

void FileSystemPanel::BeginDelete(FileEntry const& entry)
{
	m_operationBaseVirtualPath   = GetParentVirtualPath(entry.GetVirtualPath());
	m_operationTargetVirtualPath = entry.GetVirtualPath();
	m_operationTargetIsDirectory = entry.IsDirectory();
	m_operationError.clear();
	m_openDeletePopup = true;
}

void FileSystemPanel::DuplicateEntry(FileEntry const& entry, FileSystem& fileSystem)
{
	std::string duplicateVirtualPath;
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
	m_operationError = std::move(error);
	m_openErrorPopup = true;
}

bool FileSystemPanel::ValidateName(
	FileSystem const&  fileSystem,
	std::string const& parentVirtualPath,
	char const*        name,
	std::string&       outError,
	std::string const& ignoredVirtualPath) const
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
	std::string const candidate = JoinVirtualPath(parentVirtualPath, value);
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
