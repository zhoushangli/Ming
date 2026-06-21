#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <cctype>
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

std::string GetDisplayName(std::filesystem::path const& path)
{
	std::string displayName = path.filename().string();
	if (displayName.empty())
	{
		displayName = path.string();
	}
	return displayName;
}
} // namespace

FileSystemPanel::FileSystemPanel()
	: EditorPanel("FileSystem")
{
}

void FileSystemPanel::OnRender(EditorUIContext& context)
{
	ImGui::Begin(GetTitle(), GetOpenState());
	ImGui::InputTextWithHint("##FilterFiles", "Filter Files", m_filter, sizeof(m_filter));
	ImGui::Separator();

	if (context.m_fileSystem == nullptr)
	{
		ImGui::TextDisabled("FileSystem is not available.");
		ImGui::End();
		return;
	}

	std::filesystem::path const& resourceRoot = context.m_fileSystem->GetResourceRoot();
	if (!m_hasScanned)
	{
		BuildFileTree(resourceRoot);
	}

	if (!m_hasScanned)
	{
		ImGui::TextUnformatted("res://");
		ImGui::TextDisabled("Resource root not found.");
		ImGui::End();
		return;
	}

	ImGui::BeginChild("##FileSystemTreeScrollRegion", ImVec2(0.f, 0.f), false);

	ImGuiTreeNodeFlags rootFlags =
		ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
		| ImGuiTreeNodeFlags_SpanAvailWidth;
	bool const rootOpen = ImGui::TreeNodeEx("##FileSystemRoot", rootFlags);
	ImVec2 const rootMin = ImGui::GetItemRectMin();
	ImVec2 const rootMax = ImGui::GetItemRectMax();
	EditorUIWidgets::RenderTreeRowContent("Folder", "Folder", "res://", rootMin, rootMax, EditorUIStyle::FileTreeIconSize());

	if (rootOpen)
	{
		std::string const lowerFilterText = ToLower(m_filter);
		for (FileEntry const& entry : m_rootEntry.m_children)
		{
			if (!DoesEntryMatchFilter(entry, lowerFilterText))
			{
				continue;
			}

			RenderEntry(entry, lowerFilterText);
		}

		ImGui::TreePop();
	}

	ImGui::EndChild();
	ImGui::End();
}

void FileSystemPanel::BuildFileTree(std::filesystem::path const& resourceRoot)
{
	m_rootEntry = {};
	m_hasScanned = false;

	std::error_code errorCode;
	if (!std::filesystem::exists(resourceRoot, errorCode) || !std::filesystem::is_directory(resourceRoot, errorCode))
	{
		return;
	}

	m_rootEntry.m_path        = resourceRoot;
	m_rootEntry.m_name        = "res://";
	m_rootEntry.m_lowerName   = ToLower(m_rootEntry.m_name);
	m_rootEntry.m_iconName    = "Folder";
	m_rootEntry.m_isDirectory = true;

	for (std::filesystem::directory_entry const& entry : std::filesystem::directory_iterator(resourceRoot, errorCode))
	{
		std::error_code entryError;
		bool const      isDirectory = entry.is_directory(entryError);
		bool const      isFile      = entry.is_regular_file(entryError);
		if (entryError || (!isDirectory && !isFile))
		{
			continue;
		}

		m_rootEntry.m_children.push_back(BuildEntry(entry.path(), isDirectory));
	}

	std::sort(m_rootEntry.m_children.begin(), m_rootEntry.m_children.end(), [](FileEntry const& a, FileEntry const& b) {
		if (a.m_isDirectory != b.m_isDirectory)
		{
			return a.m_isDirectory && !b.m_isDirectory;
		}

		return a.m_lowerName < b.m_lowerName;
	});

	m_hasScanned = true;
}

FileSystemPanel::FileEntry FileSystemPanel::BuildEntry(std::filesystem::path const& path, bool isDirectory) const
{
	FileEntry result;
	result.m_path        = path;
	result.m_name        = GetDisplayName(path);
	result.m_lowerName   = ToLower(result.m_name);
	result.m_iconName    = GetIconNameForPath(path, isDirectory);
	result.m_isDirectory = isDirectory;

	if (isDirectory)
	{
		std::error_code errorCode;
		for (std::filesystem::directory_entry const& child : std::filesystem::directory_iterator(path, errorCode))
		{
			std::error_code entryError;
			bool const      childIsDirectory = child.is_directory(entryError);
			bool const      childIsFile      = child.is_regular_file(entryError);
			if (entryError || (!childIsDirectory && !childIsFile))
			{
				continue;
			}

			result.m_children.push_back(BuildEntry(child.path(), childIsDirectory));
		}

		std::sort(result.m_children.begin(), result.m_children.end(), [](FileEntry const& a, FileEntry const& b) {
			if (a.m_isDirectory != b.m_isDirectory)
			{
				return a.m_isDirectory && !b.m_isDirectory;
			}

			return a.m_lowerName < b.m_lowerName;
		});
	}

	return result;
}

void FileSystemPanel::RenderEntry(FileEntry const& entry, std::string const& lowerFilterText)
{
	bool const hasVisibleChildren =
		std::any_of(entry.m_children.begin(), entry.m_children.end(), [this, &lowerFilterText](FileEntry const& child) {
			return DoesEntryMatchFilter(child, lowerFilterText);
		});

	ImGuiTreeNodeFlags flags =
		ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (!entry.m_isDirectory || !hasVisibleChildren)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (!lowerFilterText.empty())
	{
		flags |= ImGuiTreeNodeFlags_DefaultOpen;
	}

	ImGui::PushID(entry.m_path.string().c_str());
	bool const isOpen = ImGui::TreeNodeEx(entry.m_isDirectory ? "##Directory" : "##File", flags);
	ImVec2 const rowMin = ImGui::GetItemRectMin();
	ImVec2 const rowMax = ImGui::GetItemRectMax();
	EditorUIWidgets::RenderTreeRowContent(
		entry.m_iconName,
		entry.m_isDirectory ? "Folder" : "File",
		entry.m_name,
		rowMin,
		rowMax,
		EditorUIStyle::FileTreeIconSize());

	if (isOpen && entry.m_isDirectory && hasVisibleChildren)
	{
		for (FileEntry const& child : entry.m_children)
		{
			if (!DoesEntryMatchFilter(child, lowerFilterText))
			{
				continue;
			}

			RenderEntry(child, lowerFilterText);
		}

		ImGui::TreePop();
	}

	ImGui::PopID();
}

bool FileSystemPanel::DoesEntryMatchFilter(FileEntry const& entry, std::string const& lowerFilterText) const
{
	if (lowerFilterText.empty())
	{
		return true;
	}

	if (entry.m_lowerName.find(lowerFilterText) != std::string::npos)
	{
		return true;
	}

	for (FileEntry const& child : entry.m_children)
	{
		if (DoesEntryMatchFilter(child, lowerFilterText))
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
