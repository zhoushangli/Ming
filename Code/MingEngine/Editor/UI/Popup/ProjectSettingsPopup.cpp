#include "MingEngine/Editor/UI/Popup/ProjectSettingsPopup.hpp"
#include "MingEngine/Core/IO/ResourceSaver.hpp"
#include "MingEngine/Core/IO/ResourceLoader.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"

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
		EditorPopupUtils::Open(
			m_popup,
			context.m_rootControl,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_height", "Editor"))));
		ObjectDatabase::GetInstance<LineEdit>(m_editID)->GrabFocus();
	}

	if (m_popup != nullptr && m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_popup,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_height", "Editor"))));
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

	if (path.IsValid() && !Ref<PackedScene>(ResourceLoader::LoadUncached(path)).IsValid())
	{
		m_error = "Start Scene must refer to an existing, valid scene.";
		m_errorLabel->SetText(m_error);
		return false;
	}

	Ref<ProjectSettings> settings = ProjectSettings::Get();
	VirtualPath const    previous = settings->m_startScenePath;
	settings->m_startScenePath    = path;
	if (!ResourceSaver::Save(ProjectSettings::GetSettingsPath(), settings))
	{
		settings->m_startScenePath = previous;
		m_error                    = "Could not save project settings to res://project.ming.";
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

	m_popup    = EditorPopupUtils::Create(context.m_rootControl, "Project Settings", m_content);
	auto* body = MemNew<HBoxContainer>();
	body->SetVerticalSizeFlags(Control::Size_ExpandFill);
	m_content->AddNode(body);
	auto* sidebar = MemNew<PanelContainer>();
	sidebar->SetThemeTypeVariation("EditorDialogSidebar");
	sidebar->AddThemeChangedCallback(
		[control = sidebar]()
		{
			control->SetCustomMinimumSize(
				Vector2(static_cast<float>(control->GetThemeConstant("node_sidebar_width", "Editor")), 0.f));
		});
	body->AddNode(sidebar);
	auto* navigation = MemNew<VBoxContainer>();
	sidebar->AddNode(navigation);
	EditorPopupUtils::AddLabel(navigation, "PROJECT", "EditorStatusLabel");
	Button* application = EditorPopupUtils::AddButton(navigation, "Application", {});
	application->SetToggleMode(true);
	application->SetPressedNoSignal(true);
	auto* fields = MemNew<VBoxContainer>();
	fields->SetHorizontalSizeFlags(Control::Size_ExpandFill);
	body->AddNode(fields);
	EditorPopupUtils::AddLabel(fields, "Application / Run", "EditorInspectorTitle");
	EditorPopupUtils::AddLabel(fields, "Start Scene");
	auto* edit = MemNew<LineEdit>();
	m_editID   = edit->GetObjectID();
	edit->SetMinimumCharacterWidth(1);
	edit->SetText(m_startScenePath);
	fields->AddNode(edit);
	EditorPopupUtils::AddButton(
		fields,
		"Browse...",
		[this, ui = context.m_editorUI]()
		{
			m_startScenePath = ObjectDatabase::GetInstance<LineEdit>(m_editID)->GetText().ToUtf8();
			m_popup->Close();
			VirtualPath initial;
			VirtualPath::TryParse(m_startScenePath, initial);
			ui->OpenSceneFile(
				SceneFilePopup::Mode::MainScene,
				initial,
				[this](VirtualPath const& path)
				{
					m_startScenePath = path.GetString();
					m_openRequested  = true;
					return true;
				},
				[this]()
				{
					m_openRequested = true;
				});
		});
	edit->SetEditingEndedCallback(
		[this](String const& text, LineEdit::EditEndReason reason)
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
	m_errorLabel  = EditorPopupUtils::AddLabel(fields, "", "EditorErrorLabel");
	auto* buttons = EditorPopupUtils::AddActions(m_content);
	EditorPopupUtils::AddButton(
		buttons,
		"Save",
		[this]()
		{
			Confirm();
		});
	EditorPopupUtils::AddButton(
		buttons,
		"Cancel",
		[this]()
		{
			Cancel();
		});
}
