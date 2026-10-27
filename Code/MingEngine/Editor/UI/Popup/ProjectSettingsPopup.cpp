#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"

#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "ThirdParty/imgui/imgui.h"

ProjectSettingsPopup::~ProjectSettingsPopup()
{
	if (LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(m_editID))
	{
		edit->SetEditingEndedCallback({});
	}

	m_host.Interrupt();
}

void ProjectSettingsPopup::Open()
{
	m_host.Interrupt();
	m_startScenePath = ProjectSettings::Get()->m_startScenePath.GetString();
	m_error.clear();
	m_openRequested = true;
	m_isOpen        = true;
	if (LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(m_editID))
	{
		edit->SetText(m_startScenePath);
	}
}

void ProjectSettingsPopup::Render(EditorUIContext& context)
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
		m_isOpen = ImGui::IsPopupOpen(popupId);
		return;
	}

	m_isOpen = true;
	PrepareFields(context);

	m_host.FinishOutsideClick();
	ImGui::TextUnformatted("Start Scene");
	m_host.Draw(Vector2(ImGui::GetContentRegionAvail().x, 36.f));
	EditorPopupUtils::ErrorText(m_error);
	EditorPopupUtils::BeginButtonRow(2);
	bool const confirm = EditorPopupUtils::ConfirmButton();
	ImGui::SameLine(0.f, 64.f);
	bool const cancel = EditorPopupUtils::CancelButton();

	if (confirm && Confirm())
	{
		ImGui::CloseCurrentPopup();
	}
	else if (cancel)
	{
		Cancel();
		ImGui::CloseCurrentPopup();
	}

	EditorPopupUtils::EndModal();
}

bool ProjectSettingsPopup::Confirm()
{
	if (LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(m_editID))
	{
		m_startScenePath = edit->GetText().ToUtf8();
		edit->EndEditing(LineEdit::EditEndReason::Submitted);
	}

	VirtualPath path;
	if (!m_startScenePath.empty() && !VirtualPath::TryParse(m_startScenePath, path))
	{
		m_error = "Start Scene must be a valid res:// path.";
		return false;
	}

	Ref<ProjectSettings> settings = ProjectSettings::Get();
	VirtualPath const    previous = settings->m_startScenePath;
	settings->m_startScenePath    = path;
	if (!ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings))
	{
		settings->m_startScenePath = previous;
		m_error                    = "Could not save project settings to res://project.ming.";
		return false;
	}

	m_error.clear();
	m_host.Interrupt();
	m_isOpen = false;
	return true;
}

void ProjectSettingsPopup::Cancel()
{
	m_host.Interrupt();
	m_isOpen = false;
}

void ProjectSettingsPopup::PrepareFields(EditorUIContext& context)
{
	m_host.Initialize(context.m_sceneTree, false);
	LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(m_editID);
	if (edit == nullptr && m_host.GetContent() != nullptr)
	{
		edit     = MemNew<LineEdit>();
		m_editID = edit->GetObjectID();
		edit->SetMinimumCharacterWidth(1);
		edit->SetText(m_startScenePath);
		edit->SetEditingEndedCallback(
			[this](String const& text, LineEdit::EditEndReason reason)
			{
				if (reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost)
				{
					m_startScenePath = text.ToUtf8();
				}
				else if (LineEdit* current = ObjectDatabase::GetInstance<LineEdit>(m_editID))
				{
					current->SetText(m_startScenePath);
				}
			});
		m_host.GetContent()->AddNode(edit);
	}

}
