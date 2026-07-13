#include "MingEngine/Editor/UI/Popup/CreateScenePopup.hpp"

#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include <algorithm>
#include <cctype>
#include <cfloat>
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

void CreateScenePopup::Open(std::string const& parentVirtualPath)
{
	m_parentVirtualPath = parentVirtualPath;
	m_selectedVirtualPath.clear();
	m_error.clear();
	m_sceneName[0]    = '\0';
	m_rootName[0]     = '\0';
	m_focusNameInput  = true;
	m_openRequested   = true;
}

void CreateScenePopup::Render(EditorUIContext& context)
{
	constexpr char const* popupId = "Create New Scene";
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
	ImGui::TextUnformatted("Root Type: 3D Scene");
	ImGui::TextUnformatted("Scene Name (.tscn):");
	if (m_focusNameInput)
	{
		ImGui::SetKeyboardFocusHere();
		m_focusNameInput = false;
	}
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool const sceneSubmitted = ImGui::InputText(
		"##CreateSceneName",
		m_sceneName,
		sizeof(m_sceneName),
		ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

	ImGui::TextUnformatted("Root Name:");
	ImGui::SetNextItemWidth(-FLT_MIN);
	bool const rootSubmitted = ImGui::InputTextWithHint(
		"##CreateSceneRootName",
		"Leave empty to use scene name",
		m_rootName,
		sizeof(m_rootName),
		ImGuiInputTextFlags_EnterReturnsTrue);

	std::string sceneError;
	std::string rootError;
	std::string const sceneName = m_sceneName;
	bool sceneValid = ValidateEntryName(m_sceneName, sceneError);
	if (sceneValid && std::filesystem::path(sceneName).has_extension())
	{
		sceneValid = false;
		sceneError = "Scene name must not include an extension.";
	}
	std::string const sceneVirtualPath = JoinVirtualPath(m_parentVirtualPath, sceneName + ".tscn");
	if (sceneValid && context.m_fileSystem != nullptr && context.m_fileSystem->Exists(sceneVirtualPath))
	{
		sceneValid = false;
		sceneError = "A scene with this name already exists.";
	}
	if (context.m_fileSystem == nullptr)
	{
		sceneValid = false;
		sceneError = "FileSystem is not available.";
	}

	std::string const rootName = m_rootName[0] != '\0' ? m_rootName : m_sceneName;
	bool const rootValid = ValidateEntryName(rootName.c_str(), rootError);
	bool const isValid   = sceneValid && rootValid;

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild(
		"##CreateSceneValidation",
		ImVec2(0.f, 0.f),
		ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
	ImGui::TextColored(
		sceneValid ? ImVec4(0.25f, 0.85f, 0.35f, 1.f) : ImVec4(0.9f, 0.35f, 0.3f, 1.f),
		"%s",
		sceneValid ? "Scene name is valid." : sceneError.c_str());
	ImGui::TextColored(
		rootValid ? ImVec4(0.25f, 0.85f, 0.35f, 1.f) : ImVec4(0.9f, 0.35f, 0.3f, 1.f),
		"%s",
		rootValid ? "Root node name is valid." : rootError.c_str());
	ImGui::EndChild();
	ImGui::PopStyleColor();
	EditorPopupUtils::ErrorText(m_error);

	EditorPopupUtils::BeginButtonRow(2);
	bool const confirm = (EditorPopupUtils::ConfirmButton() || sceneSubmitted || rootSubmitted) && isValid;
	ImGui::SameLine(0.f, 64.f);
	bool const cancel = EditorPopupUtils::CancelButton();
	if (confirm)
	{
		m_error.clear();
		if (EditorNode::Get() == nullptr)
		{
			m_error = "EditorNode is not available.";
		}
		else
		{
			EditorNode::Get()->RequestCreateScene(sceneVirtualPath, rootName);
			m_selectedVirtualPath = sceneVirtualPath;
			ImGui::CloseCurrentPopup();
		}
	}
	else if (cancel)
	{
		ImGui::CloseCurrentPopup();
	}

	EditorPopupUtils::EndModal();
}

bool CreateScenePopup::ConsumeSelectedVirtualPath(std::string& outVirtualPath)
{
	if (m_selectedVirtualPath.empty())
	{
		return false;
	}
	outVirtualPath = std::move(m_selectedVirtualPath);
	m_selectedVirtualPath.clear();
	return true;
}

bool CreateScenePopup::ValidateEntryName(char const* name, std::string& outError) const
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
