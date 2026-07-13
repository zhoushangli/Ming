#include "MingEngine/Editor/UI/Popup/CreateFolderPopup.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cstring>
#include <filesystem>
#include <utility>

namespace
{
std::string ToLower(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character)
		{ return static_cast<char>(std::tolower(character)); });
	return text;
}

bool IsReservedWindowsName(std::string const& name)
{
	std::string const stem = ToLower(std::filesystem::path(name).stem().string());
	if (stem == "con" || stem == "prn" || stem == "aux" || stem == "nul")
	{
		return true;
	}
	return stem.size() == 4 && (stem.compare(0, 3, "com") == 0 || stem.compare(0, 3, "lpt") == 0)
		   && stem[3] >= '1' && stem[3] <= '9';
}

std::string JoinVirtualPath(std::string const& parentVirtualPath, std::string const& name)
{
	return parentVirtualPath == "res://" ? parentVirtualPath + name : parentVirtualPath + "/" + name;
}
} // namespace

void CreateFolderPopup::Open(std::string const& parentVirtualPath)
{
	m_parentVirtualPath = parentVirtualPath;
	m_selectedVirtualPath.clear();
	m_error.clear();
	strcpy_s(m_name, "New Folder");
	m_focusNameInput = true;
	m_openRequested  = true;
}

void CreateFolderPopup::Render(EditorUIContext& context)
{
	constexpr char const* popupId = "Create Folder";
	if (m_openRequested)
	{
		ImGui::OpenPopup(popupId);
		m_openRequested = false;
	}

	if (!EditorPopupUtils::BeginModal(
			popupId,
			ImVec2(520.f, 0.f),
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
	{
		return;
	}

	ImGui::Text("Base path: %s", m_parentVirtualPath.c_str());
	ImGui::TextUnformatted("Name:");
	if (m_focusNameInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusNameInput = false;
	}
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool const submitted = ImGui::InputText(
		"##CreateFolderName",
		m_name,
		sizeof(m_name),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

	std::string validationError;
	bool const isValid = ValidateName(context, validationError);
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild("##CreateFolderValidation", ImVec2(0.f, 42.f), true);
	ImGui::TextColored(
		isValid ? ImVec4(0.25f, 0.85f, 0.35f, 1.f) : ImVec4(0.9f, 0.35f, 0.3f, 1.f),
		"%s",
		isValid ? "Folder name is valid." : validationError.c_str());
	ImGui::EndChild();
	ImGui::PopStyleColor();
	EditorPopupUtils::ErrorText(m_error);

	EditorPopupUtils::BeginButtonRow(2);
	bool const confirm = (EditorPopupUtils::ConfirmButton() || submitted) && isValid;
	ImGui::SameLine(0.f, 64.f);
	bool const cancel = EditorPopupUtils::CancelButton();
	if (confirm && context.m_fileSystem != nullptr)
	{
		m_error.clear();
		if (context.m_fileSystem->CreateFolder(m_parentVirtualPath, m_name, m_selectedVirtualPath, m_error))
		{
			context.m_fileSystem->ScanResourceTree();
			ImGui::CloseCurrentPopup();
		}
	}
	else if (cancel)
	{
		ImGui::CloseCurrentPopup();
	}

	EditorPopupUtils::EndModal();
}

bool CreateFolderPopup::ConsumeSelectedVirtualPath(std::string& outVirtualPath)
{
	if (m_selectedVirtualPath.empty())
	{
		return false;
	}
	outVirtualPath = std::move(m_selectedVirtualPath);
	m_selectedVirtualPath.clear();
	return true;
}

bool CreateFolderPopup::ValidateName(EditorUIContext const& context, std::string& outError) const
{
	std::string const value = m_name;
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
	if (context.m_fileSystem == nullptr)
	{
		outError = "FileSystem is not available.";
		return false;
	}
	if (context.m_fileSystem->Exists(JoinVirtualPath(m_parentVirtualPath, value)))
	{
		outError = "An entry with this name already exists.";
		return false;
	}
	outError.clear();
	return true;
}
