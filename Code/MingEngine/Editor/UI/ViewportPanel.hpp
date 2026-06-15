#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

#include "MingEngine/Core/Math/IntVec2.hpp"

struct EditorUIContext;

class ViewportPanel final : public EditorPanel
{
public:
	ViewportPanel();

private:
	void OnRender(EditorUIContext& context) override;

private:
	IntVec2 m_dimensions = IntVec2::Zero;
	bool    m_snap       = false;
};

