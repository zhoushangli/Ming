#include "MingEngine/Editor/UI/Popup/DeleteEntryPopup.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <utility>
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Engine/Application/ProjectSettings.hpp"

void DeleteEntryPopup::Open(VirtualPath const& targetVirtualPath, bool isDirectory)
{
	m_targetVirtualPath   = targetVirtualPath;
	m_parentVirtualPath   = targetVirtualPath.GetParent();
	m_isDirectory         = isDirectory;
	m_selectedVirtualPath = {};
	m_error.clear();
	m_openRequested = true;
}

void DeleteEntryPopup::Render(EditorUIContext& context)
{
	m_context = context;
	if (m_popup == nullptr && !m_openRequested)
	{
		return;
	}

	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup                = EditorPopupUtils::Create(context.m_rootControl, "Delete Entry", content);
		m_nameEdit             = MemNew<LineEdit>();
		m_nameEdit->SetEditable(false);
		content->AddNode(m_nameEdit);
		m_errorLabel  = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* buttons = EditorPopupUtils::AddActions(content);
		EditorPopupUtils::AddButton(
			buttons,
			"Delete",
			[this]()
			{
				Confirm();
			});
		EditorPopupUtils::AddButton(
			buttons,
			"Cancel",
			[this]()
			{
				m_popup->Close();
			});
	}

	if (m_openRequested)
	{
		m_openRequested        = false;
		VirtualPath const main = ProjectSettings::Get()->m_startScenePath;
		bool const        deletesMain =
			main == m_targetVirtualPath || main.GetString().rfind(m_targetVirtualPath.GetString() + "/", 0) == 0;
		m_nameEdit->SetText(
			"Delete " + m_targetVirtualPath.GetString()
			+ (deletesMain ? "? The main scene will be cleared. This cannot be undone." : "? This cannot be undone."));
		m_errorLabel->SetText("");
		EditorPopupUtils::Open(
			m_popup,
			context.m_rootControl,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("create_popup_height", "Editor"))));
	}

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(
			m_popup,
			Vector2(
				static_cast<float>(context.m_rootControl->GetThemeConstant("project_popup_width", "Editor")),
				static_cast<float>(context.m_rootControl->GetThemeConstant("create_popup_height", "Editor"))));
	}
}

bool DeleteEntryPopup::ConsumeSelectedVirtualPath(VirtualPath& outVirtualPath)
{
	if (!m_selectedVirtualPath.IsValid())
	{
		return false;
	}

	outVirtualPath        = std::move(m_selectedVirtualPath);
	m_selectedVirtualPath = {};
	return true;
}

void DeleteEntryPopup::Confirm()
{
	if (m_context.m_fileSystem != nullptr && m_context.m_fileSystem->Remove(m_targetVirtualPath, m_error))
	{
		m_context.m_fileSystem->ScanResourceTree();
		m_selectedVirtualPath = m_parentVirtualPath;
		m_popup->Close();
		if (!EditorNode::Get()->FilesRemoved(m_targetVirtualPath))
		{
			m_context.m_editorUI->Warning(
				"Project Settings",
				"Files were deleted, but the cleared main scene setting could not be saved.");
		}
	}

	m_errorLabel->SetText(m_error);
}
