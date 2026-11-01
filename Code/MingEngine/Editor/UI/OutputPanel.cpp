#include "MingEngine/Editor/UI/OutputPanel.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

OutputPanel::OutputPanel() : EditorPanel("Output")
{
	auto* page = MemNew<VBoxContainer>();
	AddNode(page);
	EditorPopupUtils::AddLabel(page, "[Info] Editor started");
}

void OutputPanel::OnRender([[maybe_unused]] EditorUIContext& context)
{
}
