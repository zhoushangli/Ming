#include "MingEngine/Editor/UI/EditorPanel.hpp"

EditorPanel::EditorPanel(char const* title)
	: m_title(title)
{
}

bool EditorPanel::IsOpen() const { return m_isOpen; }

void EditorPanel::SetOpen(bool isOpen) { m_isOpen = isOpen; }

bool* EditorPanel::GetOpenState() { return &m_isOpen; }

char const* EditorPanel::GetTitle() const { return m_title.c_str(); }
