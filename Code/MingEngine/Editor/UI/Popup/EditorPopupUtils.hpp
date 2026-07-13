#pragma once

#include "ThirdParty/imgui/imgui.h"

#include <string>

namespace EditorPopupUtils
{
bool BeginModal(char const* title, ImVec2 initialSize, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void EndModal();
void BeginButtonRow(int buttonCount, float buttonWidth = 120.f, float buttonGap = 64.f);
bool ConfirmButton(char const* label = "OK", float buttonWidth = 120.f, bool acceptEnter = true);
bool CancelButton(char const* label = "Cancel", float buttonWidth = 120.f);
void ErrorText(std::string const& error);
} // namespace EditorPopupUtils
