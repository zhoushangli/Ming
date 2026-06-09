#include "MingEngine/Editor/UI/EditorPanel.hpp"

EditorPanel::EditorPanel(char const* title, bool isOpen)
	: m_title(title)
	, m_isOpen(isOpen)
{
}

void EditorPanel::Open()
{
	UIData data;
	Open(data);
}

void EditorPanel::Open(UIData const& data)
{
	if (m_closePending)
	{
		ApplyClose();
	}

	m_isOpen = true;
	OnOpen(data);
}

void EditorPanel::Close()
{
	if (!m_isOpen || m_closePending)
	{
		return;
	}

	m_isOpen       = false;
	m_closePending = true;
}

void EditorPanel::Render(EditorUIContext& context)
{
	if (m_closePending)
	{
		ApplyClose();
		return;
	}

	if (!m_isOpen)
	{
		return;
	}

	OnRender(context);
	if (!m_isOpen && !m_closePending)
	{
		m_closePending = true;
	}
	if (m_closePending)
	{
		ApplyClose();
	}
}

void EditorPanel::OnOpen([[maybe_unused]] UIData const& data) {}

void EditorPanel::OnClose() {}

void EditorPanel::ApplyClose()
{
	m_isOpen       = false;
	m_closePending = false;
	OnClose();
}

bool EditorPanel::IsOpen() const { return m_isOpen; }

bool* EditorPanel::GetOpenState() { return &m_isOpen; }

char const* EditorPanel::GetTitle() const { return m_title.c_str(); }
