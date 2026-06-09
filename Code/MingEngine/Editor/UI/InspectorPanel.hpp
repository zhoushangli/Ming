#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class InspectorPanel final : public EditorPanel
{
public:
	InspectorPanel();

private:
	void OnRender(EditorUIContext& context) override;

private:
	float m_position[3] = { 0.f, 0.f, 0.f };
	float m_rotation[3] = { 0.f, 0.f, 0.f };
	float m_scale[3]    = { 1.f, 1.f, 1.f };
	bool  m_visible     = true;
};
