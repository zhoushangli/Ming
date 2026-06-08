#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

#include "MingEngine/Engine/Math/IntVec2.hpp"

struct EditorUIContext;

class ViewportPanel
{
public:
	ViewportPanel();

	void Render(EditorUIContext& context);

	EditorPanel&       GetPanel();
	EditorPanel const& GetPanel() const;

private:
	EditorPanel m_panel;
	IntVec2     m_dimensions = IntVec2::Zero;
	bool        m_snap       = false;
};
