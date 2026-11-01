#include "MingEngine/Editor/UI/Popup/DeleteEntryPopup.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Engine/File/FileSystem.hpp"

#include <utility>

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
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "Delete Entry", content);
		m_nameEdit = MemNew<LineEdit>();
		m_nameEdit->SetEditable(false);
		content->AddNode(m_nameEdit);
		m_errorLabel = EditorPopupUtils::AddLabel(content, "", "EditorErrorLabel");
		auto* buttons = MemNew<HBoxContainer>();
		content->AddNode(buttons);
		EditorPopupUtils::AddButton(buttons, "Delete", [this]() { Confirm(); });
		EditorPopupUtils::AddButton(buttons, "Cancel", [this]() { m_popup->Close(); });
	}

	if (m_openRequested)
	{
		m_openRequested = false;
		m_nameEdit->SetText("Delete " + m_targetVirtualPath.GetString() + "? This cannot be undone.");
		m_errorLabel->SetText("");
		EditorPopupUtils::Open(m_popup, context.m_rootControl, Vector2(650.f, 300.f));
	}

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(m_popup, Vector2(650.f, 300.f));
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
	}

	m_errorLabel->SetText(m_error);
}
