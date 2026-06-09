#pragma once

#include "MingEngine/Editor/UI/EditorPanel.hpp"

class OutputPanel final : public EditorPanel
{
public:
	OutputPanel();

private:
	void OnRender(EditorUIContext& context) override;
};
