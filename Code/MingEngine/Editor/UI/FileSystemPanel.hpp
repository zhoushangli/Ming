#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class FileSystemPanel
{
public:
	FileSystemPanel();

	void Render();

	EditorPanel&       GetPanel();
	EditorPanel const& GetPanel() const;

private:
	EditorPanel m_panel;
	char        m_filter[64] = {};
};
