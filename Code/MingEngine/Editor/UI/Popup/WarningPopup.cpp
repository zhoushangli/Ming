#include "MingEngine/Editor/UI/Popup/WarningPopup.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"

void WarningPopup::Open(std::string const& title, std::string const& message)
{
	m_title = title;
	m_message = message;
	m_openRequested = true;
}

void WarningPopup::Render(EditorUIContext& context)
{
	if (m_popup == nullptr && !m_openRequested)
	{
		return;
	}

	if (m_popup == nullptr)
	{
		VBoxContainer* content = nullptr;
		m_popup = EditorPopupUtils::Create(context.m_rootControl, "", content);
		m_heading = EditorPopupUtils::AddLabel(content, "", "EditorInspectorTitle");
		m_text = EditorPopupUtils::AddLabel(content, "");
		EditorPopupUtils::AddButton(content, "OK", [this]() { m_popup->Close(); });
	}

	if (m_openRequested)
	{
		m_openRequested = false;
		m_heading->SetText(m_title);
		m_text->SetText(m_message);
		EditorPopupUtils::Open(m_popup, context.m_rootControl, Vector2(700.f, 225.f));
	}

	if (m_popup->IsVisible())
	{
		EditorPopupUtils::Layout(m_popup, Vector2(700.f, 225.f));
	}
}
