#include "MingEngine/Editor/UI/Popup/DeleteEntryPopup.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <utility>

void DeleteEntryPopup::Open(VirtualPath const& targetVirtualPath, bool isDirectory)
{
	m_targetVirtualPath = targetVirtualPath;
	m_parentVirtualPath = targetVirtualPath.GetParent();
	m_isDirectory       = isDirectory;
	m_selectedVirtualPath = {};
	m_error.clear();
	m_openRequested = true;
}

void DeleteEntryPopup::Render(EditorUIContext& context)
{
	constexpr char const* popupId = "Delete Entry";
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

	ImGui::Text("Delete %s?", m_targetVirtualPath.CStr());
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
	ImGui::BeginChild(
		"##DeleteWarning",
		ImVec2(0.f, 0.f),
		ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_AlwaysAutoResize);
	ImGui::TextWrapped(
		m_isDirectory ? "The folder and all of its contents will be permanently deleted."
						  : "The file will be permanently deleted.");
	ImGui::TextColored(ImVec4(0.9f, 0.55f, 0.2f, 1.f), "This action does not use the Recycle Bin.");
	ImGui::EndChild();
	ImGui::PopStyleColor();
	EditorPopupUtils::ErrorText(m_error);

	EditorPopupUtils::BeginButtonRow(2);
	bool const confirm = EditorPopupUtils::ConfirmButton();
	ImGui::SameLine(0.f, 64.f);
	bool const cancel = EditorPopupUtils::CancelButton();
	if (confirm && context.m_fileSystem != nullptr)
	{
		m_error.clear();
		if (context.m_fileSystem->Remove(m_targetVirtualPath, m_error))
		{
			context.m_fileSystem->ScanResourceTree();
			m_selectedVirtualPath = m_parentVirtualPath;
			ImGui::CloseCurrentPopup();
		}
	}
	else if (cancel)
	{
		ImGui::CloseCurrentPopup();
	}

	EditorPopupUtils::EndModal();
}

bool DeleteEntryPopup::ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath)
{
	if (!m_selectedVirtualPath.IsValid())
	{
		return false;
	}
	outVirtualPath = std::move(m_selectedVirtualPath);
	m_selectedVirtualPath = {};
	return true;
}
