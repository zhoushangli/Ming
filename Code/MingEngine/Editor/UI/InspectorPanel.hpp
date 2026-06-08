#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class InspectorPanel
{
public:
	InspectorPanel();

	void Render();

	EditorPanel&       GetPanel();
	EditorPanel const& GetPanel() const;

private:
	EditorPanel m_panel;
	float       m_position[3] = { 0.f, 0.f, 0.f };
	float       m_rotation[3] = { 0.f, 0.f, 0.f };
	float       m_scale[3]    = { 1.f, 1.f, 1.f };
	bool        m_visible     = true;
};
