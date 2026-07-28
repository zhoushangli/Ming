#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

bool EditorPopupUtils::BeginModal(char const* title, ImVec2 initialSize, ImGuiWindowFlags flags)
{
	ImGuiViewport const* viewport = ImGui::GetMainViewport();
	ImVec2 const center(
		viewport->WorkPos.x + viewport->WorkSize.x * 0.5f,
		viewport->WorkPos.y + viewport->WorkSize.y * 0.5f);
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(initialSize, ImGuiCond_Appearing);

	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.f, 18.f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.f, 12.f));
	bool const isOpen = ImGui::BeginPopupModal(
		title,
		nullptr,
		flags | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
	ImGui::PopStyleColor();
	if (!isOpen)
	{
		ImGui::PopStyleVar(2);
	}
	return isOpen;
}

void EditorPopupUtils::EndModal()
{
	ImGui::EndPopup();
	ImGui::PopStyleVar(2);
}

void EditorPopupUtils::BeginButtonRow(int buttonCount, float buttonWidth, float buttonGap)
{
	float const width = buttonWidth * static_cast<float>(buttonCount)
					+ buttonGap * static_cast<float>(buttonCount - 1);
	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ImGui::GetContentRegionAvail().x - width) * 0.5f);
}

bool EditorPopupUtils::ConfirmButton(char const* label, float buttonWidth, bool acceptEnter)
{
	return ImGui::Button(label, ImVec2(buttonWidth, 0.f))
		   || (acceptEnter && ImGui::IsKeyPressed(ImGuiKey_Enter, false));
}

bool EditorPopupUtils::CancelButton(char const* label, float buttonWidth)
{
	return ImGui::Button(label, ImVec2(buttonWidth, 0.f)) || ImGui::IsKeyPressed(ImGuiKey_Escape, false);
}

void EditorPopupUtils::ErrorText(std::string const& error)
{
	if (!error.empty())
	{
		ImGui::TextColored(ImVec4(0.9f, 0.35f, 0.3f, 1.f), "%s", error.c_str());
	}
}
