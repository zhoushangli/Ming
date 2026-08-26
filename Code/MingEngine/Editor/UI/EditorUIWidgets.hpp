#pragma once

#include "ThirdParty/imgui/imgui.h"

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"
#include "MingEngine/Core/Render/Color.hpp"

#include <string>

namespace EditorUIWidgets
{
bool RenderIcon(std::string const& iconName, std::string const& fallbackIconName, ImVec2 size);

void RenderTreeRowContent(
	std::string const& iconName,
	std::string const& fallbackIconName,
	std::string const& label,
	ImVec2             rowMin,
	ImVec2             rowMax,
	ImVec2             iconSize);

void BeginPropertyRow(std::string const& label);
void NextPropertyColumn();
void EndPropertyRow();

bool PropertyBool(std::string const& label, char const* id, bool& value);
bool PropertyColor(std::string const& label, char const* id, Color& value);
bool PropertyInt(std::string const& label, char const* id, int& value);
bool PropertyFloat(std::string const& label, char const* id, float& value);
bool PropertyString(std::string const& label, char const* id, std::string& value);
bool PropertyVec3(
	std::string const& label, char const* id, Vector3& value, Vector3 const& defaultValue = Vector3::Zero);
bool PropertyEuler(std::string const& label, char const* id, EulerAngles& value);
} // namespace EditorUIWidgets
