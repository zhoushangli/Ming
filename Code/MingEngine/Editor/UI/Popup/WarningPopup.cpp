#include "MingEngine/Editor/UI/Popup/WarningPopup.hpp"

#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"

void WarningPopup::Open(std::string const& title, std::string const& message)
{
	m_title         = title;
	m_message       = message;
	m_openRequested = true;
}

void WarningPopup::Render([[maybe_unused]] EditorUIContext& context)
{
	constexpr char const* popupId = "Warning";
	if (m_openRequested)
	{
		ImGui::OpenPopup(popupId);
		m_openRequested = false;
	}

	if (!EditorPopupUtils::BeginModal(
			popupId,
			ImVec2(420.f, 0.f),
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize))
	{
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 0.72f, 0.24f, 1.f));
	ImGui::TextWrapped("WARNING: %s", m_title.c_str());
	ImGui::PopStyleColor();
	ImGui::Separator();
	ImGui::TextWrapped("%s", m_message.c_str());

	EditorPopupUtils::BeginButtonRow(1);
	if (EditorPopupUtils::ConfirmButton() || ImGui::IsKeyPressed(ImGuiKey_Escape, false))
	{
		ImGui::CloseCurrentPopup();
		m_title.clear();
		m_message.clear();
	}

	EditorPopupUtils::EndModal();
}
