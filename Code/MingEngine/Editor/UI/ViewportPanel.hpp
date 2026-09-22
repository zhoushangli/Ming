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
	bool m_snap = false;
};
