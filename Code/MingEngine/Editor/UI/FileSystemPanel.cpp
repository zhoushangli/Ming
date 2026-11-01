#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "ThirdParty/GLFW/glfw3.h"

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
		[](unsigned char character)
		{
			return static_cast<char>(std::tolower(character));
		});
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

FileSystemPanel::FileSystemPanel() : EditorPanel("FileSystem")
{
}

void FileSystemPanel::OnInitialize()
{
	auto* layout = MemNew<VBoxContainer>();
	layout->SetSeparation(5.f);
	AddNode(layout);
	layout->UpdateSize(GetSize());
	m_filter = MemNew<LineEdit>();
	m_filter->SetPlaceholder("Filter files");
	auto* tools = MemNew<HBoxContainer>();
	tools->SetSeparation(2.5f);
	layout->AddNode(tools);
	m_filter->SetHorizontalSizeFlags(Size_ExpandFill);
	tools->AddNode(m_filter);
	Button* refresh = EditorPopupUtils::AddButton(tools, "", [this]() { m_context.m_fileSystem->ScanResourceTree(); });
	refresh->SetIcon(GetThemeIcon("Reload", "EditorIcons"));
	refresh->SetThemeTypeVariation("EditorToolbarButton");
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetVerticalSizeFlags(Size_ExpandFill);
	layout->AddNode(scroll);
	m_tree = MemNew<Tree>();
	scroll->AddNode(m_tree);
	m_tree->SetSelectedCallback([this](uint64_t key)
	{
		auto found = m_fileItems.find(key);
		m_selectedVirtualPath = found != m_fileItems.end() ? found->second : VirtualPath();
	});
	m_tree->SetActivatedCallback([this](uint64_t) { OpenSelected(); });
	m_tree->SetRenamedCallback([this](uint64_t key, String const& name)
	{
		auto found = m_fileItems.find(key);
		if (found == m_fileItems.end() || found->second.IsRoot())
		{
			return;
		}

		std::string error;
		if (!ValidateName(*m_context.m_fileSystem, found->second.GetParent(), name.ToUtf8().c_str(), error, found->second))
		{
			ShowError(error);
			return;
		}

		VirtualPath renamed;
		if (m_context.m_fileSystem->Rename(found->second, name.ToUtf8(), renamed, error))
		{
			m_selectedVirtualPath = renamed;
			m_context.m_fileSystem->ScanResourceTree();
		}
		else
		{
			ShowError(error);
		}
	});
	m_tree->SetInputCallback([this](InputEvent const& event) { return HandleTreeInput(event); });
	m_tree->SetDragCallbacks(
		[this](Vector2 const& position, GuiDragData& data)
		{
			auto found = m_fileItems.find(m_tree->GetItemAtPosition(position));
			if (found == m_fileItems.end() || found->second.IsRoot())
			{
				return false;
			}

			data = { "file", ObjectID::Invalid, found->second.GetString() };
			m_context.m_editorUI->SetNativeDragPreview(m_tree, data.m_value);
			return true;
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			VirtualPath source;
			return data.m_type == String("file") && VirtualPath::TryParse(data.m_value.ToUtf8(), source)
				&& CanMove(source, m_tree->GetItemAtPosition(position));
		},
		[this](Vector2 const& position, GuiDragData const& data)
		{
			VirtualPath source;
			uint64_t const target = m_tree->GetItemAtPosition(position);
			if (!VirtualPath::TryParse(data.m_value.ToUtf8(), source) || !CanMove(source, target))
			{
				return;
			}

			VirtualPath moved;
			std::string error;
			if (m_context.m_fileSystem->Move(source, m_fileItems.at(target), moved, error))
			{
				m_selectedVirtualPath = moved;
				m_context.m_fileSystem->ScanResourceTree();
			}
			else
			{
				ShowError(error);
			}
		});
}

void FileSystemPanel::OnRender(EditorUIContext& context)
{
	m_createFolderPopup.Render(context);
	m_createScenePopup.Render(context);
	m_deleteEntryPopup.Render(context);
	VirtualPath selected;
	if (m_createFolderPopup.ConsumeSelectedVirtualPath(selected)
		|| m_createScenePopup.ConsumeSelectedVirtualPath(selected)
		|| m_deleteEntryPopup.ConsumeSelectedVirtualPath(selected))
	{
		m_selectedVirtualPath = selected;
	}

	UpdateItems();
}

FileEntry const* FileSystemPanel::GetSelectedEntry() const
{
	FileEntry const* root = m_context.m_fileSystem->GetResourceRootEntry();
	return root != nullptr ? FindEntry(*root, m_selectedVirtualPath) : nullptr;
}

void FileSystemPanel::OpenSelected()
{
	if (FileEntry const* entry = GetSelectedEntry())
	{
		if (entry->IsDirectory())
		{
			m_tree->SetCollapsed(m_tree->GetSelected(), false);
		}
		else if (entry->GetVirtualPath().HasExtension(".tscn"))
		{
			m_context.m_editorUI->FinishNativeEditing();
			EditorNode::Get()->RequestLoadScene(entry->GetVirtualPath());
		}
	}
}

void FileSystemPanel::DeleteSelected()
{
	if (FileEntry const* entry = GetSelectedEntry(); entry != nullptr && !entry->GetVirtualPath().IsRoot())
	{
		m_deleteEntryPopup.Open(entry->GetVirtualPath(), entry->IsDirectory());
	}
}

bool FileSystemPanel::HandleTreeInput(InputEvent const& event)
{
	switch (event.m_type)
	{
	case InputEventType::Key:
		if (event.m_pressed && event.m_keyCode == GLFW_KEY_DELETE)
		{
			DeleteSelected();
			return true;
		}
		break;
	case InputEventType::MouseButton:
		if (event.m_pressed && event.m_mouseButton == GLFW_MOUSE_BUTTON_RIGHT)
		{
			m_tree->Select(m_tree->GetItemAtPosition(event.m_position), true);
			ShowContextMenu(event.m_position + m_tree->GetGlobalPosition());
			return true;
		}
		break;
	}

	return false;
}

void FileSystemPanel::ShowContextMenu(Vector2 position)
{
	FileEntry const* entry = GetSelectedEntry();
	VirtualPath const directory = entry == nullptr ? VirtualPath("res://")
		: entry->IsDirectory() ? entry->GetVirtualPath() : entry->GetVirtualPath().GetParent();
	std::vector<EditorUI::MenuItem> actions = {
		{ "Open", [this]() { OpenSelected(); } },
		{ "New Folder...", [this, directory]() { m_createFolderPopup.Open(directory); } },
		{ "New Scene...", [this, directory]() { m_createScenePopup.Open(directory); } }
	};
	if (entry != nullptr)
	{
		VirtualPath const path = entry->GetVirtualPath();
		std::filesystem::path const physical = entry->GetPhysicalPath();
		actions.push_back({ "Copy Path", [path]() { g_engine->m_windowSystem->SetClipboardText(path.GetString()); } });
		actions.push_back({ "Copy Absolute Path", [physical]() { g_engine->m_windowSystem->SetClipboardText(physical.string()); } });
		if (!path.IsRoot())
		{
			actions.push_back({ "Rename", [this]() { m_tree->EditSelected(); } });
			actions.push_back({ "Duplicate", [this, path]()
			{
				VirtualPath duplicate;
				std::string error;
				if (m_context.m_fileSystem->Duplicate(path, duplicate, error))
				{
					m_selectedVirtualPath = duplicate;
					m_context.m_fileSystem->ScanResourceTree();
				}
				else
				{
					ShowError(error);
				}
			} });
			actions.push_back({ "Delete...", [this]() { DeleteSelected(); } });
		}

		if (entry->IsDirectory())
		{
			actions.push_back({ "Open in Terminal", [this, physical]() { OpenInTerminal(physical); } });
		}
		actions.push_back({ "Open in File Manager", [this, physical, directory = entry->IsDirectory()]()
		{
			OpenInFileManager(physical, !directory);
		} });
	}

	m_context.m_editorUI->ShowMenu(m_tree, position, actions);
}

bool FileSystemPanel::CanMove(VirtualPath const& source, uint64_t target) const
{
	auto found = m_fileItems.find(target);
	if (found == m_fileItems.end() || !source.IsValid() || source.IsRoot() || source == found->second
		|| source.GetParent() == found->second)
	{
		return false;
	}

	std::filesystem::path from;
	std::filesystem::path to;
	std::error_code error;
	if (!m_context.m_fileSystem->TryGetPhysicalPath(source, from)
		|| !m_context.m_fileSystem->TryGetPhysicalPath(found->second, to)
		|| !std::filesystem::exists(from, error) || error || !std::filesystem::is_directory(to, error) || error)
	{
		return false;
	}

	std::string const prefix = source.GetString() + "/";
	return found->second.GetString().compare(0, prefix.size(), prefix) != 0
		&& !std::filesystem::exists(to / from.filename(), error) && !error;
}

void FileSystemPanel::UpdateItems()
{
	std::string const filter = ToLower(m_filter->GetText().ToUtf8());
	std::vector<Tree::Item> items;
	m_fileItems.clear();
	std::function<bool(FileEntry const*, uint64_t)> append = [&](FileEntry const* entry, uint64_t parent)
	{
		auto result = m_fileKeys.emplace(entry->GetVirtualPath().GetString(), m_nextFileKey);
		if (result.second)
		{
			++m_nextFileKey;
		}

		uint64_t const key = result.first->second;
		m_fileItems[key] = entry->GetVirtualPath();
		size_t const index = items.size();
		std::string const extension = ToLower(entry->GetPhysicalPath().extension().string());
		char const* icon = entry->IsDirectory() ? "Folder" : extension == ".tscn" ? "PackedScene"
			: extension == ".cs" ? "CSharpScript" : extension == ".png" || extension == ".jpg" ? "Texture2D"
			: extension == ".obj" || extension == ".glb" ? "Mesh" : "File";
		items.push_back({ key, parent, entry->GetName(), true, !entry->GetVirtualPath().IsRoot(),
			GetThemeIcon(icon, "EditorIcons"), EditorIcons::GetIconColor(icon, this) });
		bool visible = filter.empty() || entry->GetLowerName().find(filter) != std::string::npos;
		for (auto const& child : entry->GetChildren())
		{
			visible = append(child.get(), key) || visible;
		}

		items[index].m_visible = visible;
		return visible;
	};
	if (FileEntry const* root = m_context.m_fileSystem->GetResourceRootEntry())
	{
		append(root, 0);
	}

	m_tree->SetItems(std::move(items), !filter.empty());
	auto selected = m_fileKeys.find(m_selectedVirtualPath.GetString());
	m_tree->Select(selected != m_fileKeys.end() ? selected->second : 0);
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
