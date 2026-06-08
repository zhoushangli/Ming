#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class OutputPanel
{
public:
	OutputPanel();

	void Render();

	EditorPanel&       GetPanel();
	EditorPanel const& GetPanel() const;

private:
	EditorPanel m_panel;
};
