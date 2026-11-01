#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"

ProjectSettingsPopup::~ProjectSettingsPopup()
{
	if (LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(m_editID))
	{
		edit->SetEditingEndedCallback({});
	}
}

void ProjectSettingsPopup::Open()
{
	m_startScenePath = ProjectSettings::Get()->m_startScenePath.GetString();
	m_error.clear();
	m_openRequested = true;
}

bool ProjectSettingsPopup::IsOpen() const
{
	return m_openRequested || (m_popup != nullptr && m_popup->IsVisible());
}

void ProjectSettingsPopup::Render(EditorUIContext& context)
{
	if (m_openRequested)
	{
		PrepareFields(context);
		m_openRequested = false;
		ObjectDatabase::GetInstance<LineEdit>(m_editID)->SetText(m_startScenePath);
		m_errorLabel->SetText("");
		EditorPopupUtils::Open(m_popup, context.m_rootControl, Vector2(650.f, 275.f));
		ObjectDatabase::GetInstance<LineEdit>(m_editID)->GrabFocus();
	}

	if (m_popup != nullptr && m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(m_popup, Vector2(650.f, 275.f));
	}
}

bool ProjectSettingsPopup::Confirm()
{
	if (LineEdit* edit = ObjectDatabase::GetInstance<LineEdit>(m_editID))
	{
		m_startScenePath = edit->GetText().ToUtf8();
		edit->EndEditing(LineEdit::EditEndReason::Interrupted);
	}

	VirtualPath path;
	if (!m_startScenePath.empty() && !VirtualPath::TryParse(m_startScenePath, path))
	{
		m_error = "Start Scene must be a valid res:// path.";
		m_errorLabel->SetText(m_error);
		return false;
	}

	Ref<ProjectSettings> settings = ProjectSettings::Get();
	VirtualPath const    previous = settings->m_startScenePath;
	settings->m_startScenePath    = path;
	if (!ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings))
	{
		settings->m_startScenePath = previous;
		m_error = "Could not save project settings to res://project.ming.";
		m_errorLabel->SetText(m_error);
		return false;
	}

	m_error.clear();
	if (m_popup != nullptr)
	{
		m_popup->Close();
	}
	return true;
}

void ProjectSettingsPopup::Cancel()
{
	m_openRequested = false;
	if (m_popup != nullptr)
	{
		m_popup->Close();
	}
}

void ProjectSettingsPopup::PrepareFields(EditorUIContext& context)
{
	if (m_popup != nullptr)
	{
		return;
	}

	m_popup = EditorPopupUtils::Create(context.m_rootControl, "Project Settings", m_content);
	EditorPopupUtils::AddLabel(m_content, "Start Scene");
	auto* edit = MemNew<LineEdit>();
	m_editID = edit->GetObjectID();
	edit->SetMinimumCharacterWidth(1);
	edit->SetText(m_startScenePath);
	m_content->AddNode(edit);
	edit->SetEditingEndedCallback([this](String const& text, LineEdit::EditEndReason reason)
	{
		if (reason == LineEdit::EditEndReason::Submitted || reason == LineEdit::EditEndReason::FocusLost)
		{
			m_startScenePath = text.ToUtf8();
			if (reason == LineEdit::EditEndReason::Submitted)
			{
				Confirm();
			}
		}
	});
	m_errorLabel = EditorPopupUtils::AddLabel(m_content, "", "EditorErrorLabel");
	auto* buttons = MemNew<HBoxContainer>();
	m_content->AddNode(buttons);
	EditorPopupUtils::AddButton(buttons, "Save", [this]() { Confirm(); });
	EditorPopupUtils::AddButton(buttons, "Cancel", [this]() { Cancel(); });
}
