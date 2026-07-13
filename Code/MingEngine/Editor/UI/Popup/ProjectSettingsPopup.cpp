#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"

#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"

void ProjectSettingsPopup::Open()
{
	m_startScenePath = ProjectSettings::Get()->m_startScenePath;
	m_error.clear();
	m_openRequested = true;
}

void ProjectSettingsPopup::Render([[maybe_unused]] EditorUIContext& context)
{
	constexpr char const* popupId = "Project Settings";
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

	EditorUIWidgets::PropertyString("Start Scene", "##StartScene", m_startScenePath);
	EditorPopupUtils::ErrorText(m_error);
	EditorPopupUtils::BeginButtonRow(2);
	bool const confirm = EditorPopupUtils::ConfirmButton();
	ImGui::SameLine(0.f, 64.f);
	bool const cancel = EditorPopupUtils::CancelButton();

	if (confirm)
	{
		Ref<ProjectSettings> settings          = ProjectSettings::Get();
		std::string const    oldStartScenePath = settings->m_startScenePath;
		settings->m_startScenePath             = m_startScenePath;
		if (ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings))
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			settings->m_startScenePath = oldStartScenePath;
			m_error = "Could not save project settings to res://project.ming.";
		}
	}
	else if (cancel)
	{
		ImGui::CloseCurrentPopup();
	}

	EditorPopupUtils::EndModal();
}
