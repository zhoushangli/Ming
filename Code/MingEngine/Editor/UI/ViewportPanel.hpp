#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"
#include "MingEngine/Scene/GUI/SubViewportContainer.hpp"

class Label;

class ViewportPanel final : public EditorPanel
{
	MCLASS(ViewportPanel, EditorPanel);

public:
	ViewportPanel();
	SubViewportContainer* GetSceneView() const { return m_view; }
	bool IsNavigating() const { return m_navigating; }
	void CancelNavigation() { m_navigating = false; }

protected:
	void OnInitialize() override;
	void OnRender(EditorUIContext& context) override;

private:
	bool HandleSceneInput(InputEvent const& event);

private:
	Label* m_sceneTitle = nullptr;
	SubViewportContainer* m_view = nullptr;
	bool m_navigating = false;
};
