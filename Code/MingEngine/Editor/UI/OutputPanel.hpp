#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class OutputPanel final : public EditorPanel
{
	MCLASS(OutputPanel, EditorPanel);

public:
	OutputPanel();

private:
	void OnRender(EditorUIContext& context) override;
};
