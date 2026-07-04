#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <cctype>
#include <cstring>
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
} // namespace

FileSystemPanel::FileSystemPanel() : EditorPanel("FileSystem") {}

std::string const& FileSystemPanel::GetSelectedVirtualPath() const
{
	return m_selectedVirtualPath;
}

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
	RenderEntry(*rootEntry, lowerFilterText);

	ImGui::EndChild();
	m_wasFocused = isFocused;
	ImGui::End();
}

void FileSystemPanel::RenderEntry(FileEntry const& entry, std::string const& lowerFilterText)
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

			RenderEntry(*child, lowerFilterText);
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
