#pragma once

#include <string_view>

namespace DefaultImGuiLayout
{
// Return the built-in initial editor layout.
// e.g. ImGui::LoadIniSettingsFromMemory(DefaultImGuiLayout::Data.data(), DefaultImGuiLayout::Data.size())
extern std::string_view const Data;
} // namespace DefaultImGuiLayout