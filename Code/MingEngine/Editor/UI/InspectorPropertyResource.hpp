#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <string>

class InspectorPropertyResource final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(Variant const& value) override
	{
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();

		RenderResourcePicker(value);

		ImGui::Columns(1);
	}

private:
	void RenderResourcePicker(Variant const& value)
	{
		constexpr float rowHeight       = 36.f;
		constexpr float sideButtonWidth = 24.f;
		constexpr float spacing         = 2.f;

		float const availableWidth = ImGui::GetContentRegionAvail().x;
		float const pickerWidth    = std::max(1.f, availableWidth - sideButtonWidth - spacing);
		Resource const* resource = GetResource(value);
		bool const  hasResource    = resource != nullptr;

		if (hasResource)
		{
			RenderRevertPlaceholder(rowHeight);
			ImGui::SameLine(0.f, spacing);
		}

		float const occupiedWidth = hasResource ? rowHeight + spacing : 0.f;
		float const fieldWidth    = std::max(1.f, pickerWidth - occupiedWidth);
		RenderAssignButton(ImVec2(fieldWidth, rowHeight), resource);

		ImGui::SameLine(0.f, spacing);
		RenderExpandButton(ImVec2(sideButtonWidth, rowHeight));
	}

	void RenderRevertPlaceholder(float rowHeight) const
	{
		ImVec2 const pos = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton((m_labelId + "_revert").c_str(), ImVec2(rowHeight, rowHeight));

		ImDrawList*  drawList = ImGui::GetWindowDrawList();
		ImVec2 const center(pos.x + rowHeight * 0.5f, pos.y + rowHeight * 0.5f);
		float const  radius = rowHeight * 0.24f;
		ImU32 const  color  = ImGui::GetColorU32(ImGuiCol_TextDisabled);
		drawList->PathArcTo(center, radius, 3.8f, 0.6f, 16);
		drawList->PathStroke(color, false, 1.7f);
		drawList->AddTriangleFilled(
			ImVec2(center.x + radius + 1.f, center.y - 1.f),
			ImVec2(center.x + radius - 4.f, center.y - 5.f),
			ImVec2(center.x + radius - 2.f, center.y + 2.f),
			color);
	}

	void RenderAssignButton(ImVec2 buttonSize, Resource const* resource)
	{
		std::string const buttonId = m_labelId + "_resource";
		ImGui::InvisibleButton(buttonId.c_str(), buttonSize);

		ImVec2 const min         = ImGui::GetItemRectMin();
		ImVec2 const max         = ImGui::GetItemRectMax();
		bool const   hovered     = ImGui::IsItemHovered();
		bool         dropPreview = false;

		if (ImGui::BeginDragDropTarget())
		{
			if (ImGuiPayload const* payload =
					ImGui::AcceptDragDropPayload("FILESYSTEM_RESOURCE", ImGuiDragDropFlags_AcceptBeforeDelivery))
			{
				if (payload->DataSize == sizeof(FilePayload) && payload->Data != nullptr)
				{
					FilePayload const& data = *static_cast<FilePayload const*>(payload->Data);

					if (data.m_resource.IsValid() && data.m_resource->GetClassName() == m_info.m_hintData)
					{
						dropPreview = true;
						if (payload->IsDelivery())
						{
							// When we pass a Ref<> to Variant
							// actually we are just pass in the reference pointer
							EmitValueChanged(Variant(data.m_resource));
						}
					}
				}
			}
			ImGui::EndDragDropTarget();
		}

		DrawAssignButton(min, max, hovered, dropPreview, resource);

		if (hovered)
		{
			if (resource == nullptr || resource->GetVirtualPath().empty())
			{
				ImGui::SetTooltip("Type: %s", m_info.m_hintData.c_str());
			}
			else
			{
				ImGui::SetTooltip("%s\nType: %s", resource->GetVirtualPath().c_str(), m_info.m_hintData.c_str());
			}
		}
	}

	void DrawAssignButton(ImVec2 min, ImVec2 max, bool hovered, bool dropPreview, Resource const* resource) const
	{
		ImDrawList*  drawList = ImGui::GetWindowDrawList();
		ImVec4 const color    = dropPreview ? EditorUIStyle::ControlBackgroundActiveColor()
								: hovered   ? EditorUIStyle::ControlBackgroundHoveredColor()
											: EditorUIStyle::ControlBackgroundColor();
		drawList->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color), 3.f);

		float const textHeight = ImGui::GetTextLineHeight();
		float const textY      = min.y + (max.y - min.y - textHeight) * 0.5f;
		ImVec2      textPos(min.x + 8.f, textY);
		std::string text      = "<empty>";
		ImU32       textColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

		if (dropPreview)
		{
			text      = "Drop " + m_info.m_hintData + " here";
			textColor = ImGui::GetColorU32(ImGuiCol_Text);
		}
		else if (resource != nullptr)
		{
			ImVec2 const      iconSize(16.f, 16.f);
			float const       iconY     = min.y + (max.y - min.y - iconSize.y) * 0.5f;
			ImTextureID const textureId = EditorIcons::GetClassIconId(m_info.m_hintData);
			EditorIcons::AddImage(
				drawList,
				textureId,
				ImVec2(min.x + 7.f, iconY),
				ImVec2(min.x + 7.f + iconSize.x, iconY + iconSize.y));
			text      = resource->GetName();
			textColor = ImGui::GetColorU32(ImGuiCol_Text);
			textPos.x += iconSize.x + 6.f;
		}

		if (!dropPreview && resource == nullptr)
		{
			ImVec2 const      iconSize(16.f, 16.f);
			float const       iconY     = min.y + (max.y - min.y - iconSize.y) * 0.5f;
			ImTextureID const textureId = EditorIcons::GetIconId("Folder");
			EditorIcons::AddImage(
				drawList,
				textureId,
				ImVec2(max.x - iconSize.x - 7.f, iconY),
				ImVec2(max.x - 7.f, iconY + iconSize.y));
		}

		float const  rightPadding = resource == nullptr && !dropPreview ? 30.f : 8.f;
		ImVec2 const clipMin(textPos.x, min.y);
		ImVec2 const clipMax(max.x - rightPadding, max.y);
		drawList->PushClipRect(clipMin, clipMax, true);
		drawList->AddText(textPos, textColor, text.c_str());
		drawList->PopClipRect();
	}

	void RenderExpandButton(ImVec2 buttonSize) const
	{
		std::string const buttonId = m_labelId + "_menu";
		ImGui::InvisibleButton(buttonId.c_str(), buttonSize);

		ImVec2 const min      = ImGui::GetItemRectMin();
		ImVec2 const max      = ImGui::GetItemRectMax();
		bool const   hovered  = ImGui::IsItemHovered();
		ImDrawList*  drawList = ImGui::GetWindowDrawList();
		ImVec4 const color =
			hovered ? EditorUIStyle::ControlBackgroundHoveredColor() : EditorUIStyle::ControlBackgroundColor();
		drawList->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color), 3.f);

		ImU32 const  arrowColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);
		ImVec2 const center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
		drawList->AddTriangleFilled(
			ImVec2(center.x - 4.f, center.y - 2.f),
			ImVec2(center.x + 4.f, center.y - 2.f),
			ImVec2(center.x, center.y + 3.f),
			arrowColor);
	}

	Resource const* GetResource(Variant const& value) const
	{
		if (!value.Is<Object*>())
		{
			return nullptr;
		}

		return dynamic_cast<Resource const*>(value.As<Object*>());
	}
};
