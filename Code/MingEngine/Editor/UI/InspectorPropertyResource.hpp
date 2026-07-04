#pragma once

#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <filesystem>
#include <string>

class InspectorPropertyResource final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext& context, Variant const& value) override
	{
		ImGui::Columns(2, nullptr, false);
		ImGui::TextUnformatted(GetDisplayName().c_str());
		ImGui::NextColumn();

		RenderResourcePicker(context, value);

		ImGui::Columns(1);
	}

private:
	struct DrawRect
	{
		ImVec2 m_min;
		ImVec2 m_max;
	};

	void RenderResourcePicker(EditorUIContext& context, Variant const& value)
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
			RenderReloadButton(rowHeight);
			ImGui::SameLine(0.f, spacing);
		}

		float const occupiedWidth = hasResource ? rowHeight + spacing : 0.f;
		float const fieldWidth    = std::max(1.f, pickerWidth - occupiedWidth);
		bool const dropPreview = IsDraggedResourceMatching();
		DrawRect const fieldRect =
			RenderAssignButton(context, ImVec2(fieldWidth, rowHeight), resource, dropPreview);

		ImGui::SameLine(0.f, spacing);
		DrawRect const expandRect = RenderExpandButton(ImVec2(sideButtonWidth, rowHeight));

		if (dropPreview)
		{
			// Draw last so the drop outline stays above the expand icon when their pixels overlap.
			DrawDropPreviewOutline(fieldRect.m_min, expandRect.m_max);
		}
	}

	void RenderReloadButton(float rowHeight) const
	{
		ImGui::InvisibleButton((m_labelId + "_reload").c_str(), ImVec2(rowHeight, rowHeight));

		ImVec2 const min     = ImGui::GetItemRectMin();
		ImVec2 const max     = ImGui::GetItemRectMax();
		bool const   hovered = ImGui::IsItemHovered();
		ImDrawList*  drawList = ImGui::GetWindowDrawList();
		ImVec4 const color =
			hovered ? EditorUIStyle::ControlBackgroundHoveredColor() : EditorUIStyle::ControlBackgroundColor();
		drawList->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color), 3.f);

		ImTextureID const textureId = EditorIcons::GetIconId("Reload");
		if (textureId != ImTextureID{})
		{
			ImVec2 const iconSize(16.f, 16.f);
			ImVec2 const center((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f);
			ImVec2 const iconMin(center.x - iconSize.x * 0.5f, center.y - iconSize.y * 0.5f);
			EditorIcons::AddImage(drawList, textureId, iconMin, ImVec2(iconMin.x + iconSize.x, iconMin.y + iconSize.y));
		}

		if (hovered)
		{
			ImGui::SetTooltip("Reload");
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			EmitValueChanged(Variant(static_cast<Object*>(nullptr)));
		}
	}

	DrawRect RenderAssignButton(
		EditorUIContext& context, ImVec2 buttonSize, Resource const* resource, bool dropPreview)
	{
		std::string const buttonId = m_labelId + "_resource";
		ImGui::InvisibleButton(buttonId.c_str(), buttonSize);

		ImVec2 const min         = ImGui::GetItemRectMin();
		ImVec2 const max         = ImGui::GetItemRectMax();
		bool const   hovered     = ImGui::IsItemHovered();

		if (dropPreview && context.m_editorUI != nullptr && ImGui::IsMouseHoveringRect(min, max))
		{
			context.m_editorUI->SetResourceDropAllowed(true);
		}

		if (ImGui::BeginDragDropTarget())
		{
			ImGuiDragDropFlags const flags =
				ImGuiDragDropFlags_AcceptBeforeDelivery | ImGuiDragDropFlags_AcceptNoDrawDefaultRect;
			if (ImGuiPayload const* payload =
					ImGui::AcceptDragDropPayload("FILESYSTEM_RESOURCE", flags))
			{
				if (payload->DataSize == sizeof(FilePayload) && payload->Data != nullptr)
				{
					FilePayload const& data = *static_cast<FilePayload const*>(payload->Data);

					if (data.m_resource.IsValid() && data.m_resource->GetClassName() == m_info.m_hintData)
					{
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

		DrawAssignButton(min, max, hovered, resource);

		if (hovered)
		{
			if (resource == nullptr)
			{
				ImGui::SetTooltip("Type: %s", m_info.m_hintData.c_str());
			}
			else
			{
				SetResourceTooltip(*resource);
			}
		}

		return DrawRect{min, max};
	}

	void DrawAssignButton(ImVec2 min, ImVec2 max, bool hovered, Resource const* resource) const
	{
		ImDrawList*  drawList = ImGui::GetWindowDrawList();
		ImVec4 const color = hovered ? EditorUIStyle::ControlBackgroundHoveredColor()
									 : EditorUIStyle::ControlBackgroundColor();
		drawList->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(color), 3.f);

		float const textHeight = ImGui::GetTextLineHeight();
		float const textY      = min.y + (max.y - min.y - textHeight) * 0.5f;
		ImVec2      textPos(min.x + 8.f, textY);
		std::string text      = "<empty>";
		ImU32       textColor = ImGui::GetColorU32(ImGuiCol_TextDisabled);

		if (resource != nullptr)
		{
			ImVec2 const      iconSize(16.f, 16.f);
			float const       iconY     = min.y + (max.y - min.y - iconSize.y) * 0.5f;
			ImTextureID const textureId = EditorIcons::GetClassIconId(m_info.m_hintData);
			EditorIcons::AddImage(
				drawList,
				textureId,
				ImVec2(min.x + 7.f, iconY),
				ImVec2(min.x + 7.f + iconSize.x, iconY + iconSize.y));
			text      = GetResourceDisplayName(*resource);
			textColor = ImGui::GetColorU32(ImGuiCol_Text);
			textPos.x += iconSize.x + 6.f;
		}

		if (resource == nullptr)
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

		float const  rightPadding = resource == nullptr ? 30.f : 8.f;
		ImVec2 const clipMin(textPos.x, min.y);
		ImVec2 const clipMax(max.x - rightPadding, max.y);
		drawList->PushClipRect(clipMin, clipMax, true);
		drawList->AddText(textPos, textColor, text.c_str());
		drawList->PopClipRect();
	}

	DrawRect RenderExpandButton(ImVec2 buttonSize) const
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

		return DrawRect{min, max};
	}

	void DrawDropPreviewOutline(ImVec2 min, ImVec2 max) const
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImU32 const color    = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
		drawList->AddRect(min, max, color, 3.f, 0, 2.5f);
	}

	Resource const* GetResource(Variant const& value) const
	{
		if (!value.Is<Object*>())
		{
			return nullptr;
		}

		return dynamic_cast<Resource const*>(value.As<Object*>());
	}

	bool IsDraggedResourceMatching() const
	{
		ImGuiPayload const* payload = ImGui::GetDragDropPayload();
		if (payload == nullptr || !payload->IsDataType("FILESYSTEM_RESOURCE"))
		{
			return false;
		}
		if (payload->DataSize != sizeof(FilePayload) || payload->Data == nullptr)
		{
			return false;
		}

		FilePayload const& data = *static_cast<FilePayload const*>(payload->Data);
		return data.m_resource.IsValid() && data.m_resource->GetClassName() == m_info.m_hintData;
	}

	std::string GetResourceDisplayName(Resource const& resource) const
	{
		std::string const& sourcePath = resource.GetSourceFilePath();
		if (!sourcePath.empty())
		{
			return std::filesystem::path(sourcePath).filename().string();
		}

		std::string const& virtualPath = resource.GetVirtualPath();
		if (!virtualPath.empty())
		{
			return std::filesystem::path(virtualPath).filename().string();
		}

		return resource.GetName();
	}

	void SetResourceTooltip(Resource const& resource) const
	{
		std::string const& sourcePath = resource.GetSourceFilePath();
		std::string const& virtualPath = resource.GetVirtualPath();

		if (!sourcePath.empty() && !virtualPath.empty() && sourcePath != virtualPath)
		{
			ImGui::SetTooltip(
				"Source: %s\nResource: %s\nType: %s",
				sourcePath.c_str(),
				virtualPath.c_str(),
				m_info.m_hintData.c_str());
		}
		else if (!sourcePath.empty())
		{
			ImGui::SetTooltip("Source: %s\nType: %s", sourcePath.c_str(), m_info.m_hintData.c_str());
		}
		else if (!virtualPath.empty())
		{
			ImGui::SetTooltip("Resource: %s\nType: %s", virtualPath.c_str(), m_info.m_hintData.c_str());
		}
		else
		{
			ImGui::SetTooltip("Type: %s", m_info.m_hintData.c_str());
		}
	}
};
