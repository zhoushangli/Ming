#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Editor/UI/EditorUIStyle.hpp"
#include "MingEngine/Editor/UI/EditorUIWidgets.hpp"
#include "MingEngine/Editor/UI/FileSystemPanel.hpp"
#include "MingEngine/Editor/UI/InspectorProperty.hpp"

#include "ThirdParty/imgui/imgui.h"

#include <algorithm>
#include <filesystem>
#include <string>

class InspectorPropertyResource final : public InspectorProperty
{
public:
	using InspectorProperty::InspectorProperty;

	void RenderValue(EditorUIContext&, Variant const& value) override
	{
		ImVec2 const labelPos = ImGui::GetCursorScreenPos();
		EditorUIWidgets::BeginPropertyRow(GetDisplayName());

		if (GetResource(value) != nullptr)
		{
			RenderResetButton(labelPos.y);
		}

		EditorUIWidgets::NextPropertyColumn();

		RenderResourcePicker(value);

		EditorUIWidgets::EndPropertyRow();
	}

private:
	struct DrawRect
	{
		ImVec2 m_min;
		ImVec2 m_max;
	};

	void RenderResourcePicker(Variant const& value)
	{
		constexpr float rowHeight       = 36.f;
		constexpr float sideButtonWidth = 24.f;
		constexpr float spacing         = 5.f;

		float const     availableWidth = ImGui::GetContentRegionAvail().x;
		float const     fieldWidth     = std::max(1.f, availableWidth - sideButtonWidth - spacing);
		Resource const* resource       = GetResource(value);

		DrawRect const fieldRect = RenderAssignButton(ImVec2(fieldWidth, rowHeight), resource);
		ImGui::SameLine(0.f, spacing);
		DrawRect const expandRect = RenderExpandButton(ImVec2(sideButtonWidth, rowHeight));
	}

	void RenderResetButton(float y) const
	{
		float const  buttonSize = ImGui::GetTextLineHeight();
		float const  rightEdge  = ImGui::GetWindowPos().x + ImGui::GetColumnOffset(1);
		ImVec2 const buttonPos(rightEdge - buttonSize - ImGui::GetStyle().ItemSpacing.x, y);

		ImGui::SetCursorScreenPos(buttonPos);
		bool const clicked = ImGui::InvisibleButton((m_labelId + "_reset").c_str(), ImVec2(buttonSize, buttonSize));

		ImTextureID const textureId = EditorIcons::GetIconId("ReloadSmall", "Reload");
		if (textureId != ImTextureID{})
		{
			constexpr float iconInset = 2.f;
			EditorIcons::AddImage(
				ImGui::GetWindowDrawList(),
				textureId,
				ImVec2(buttonPos.x + iconInset, buttonPos.y + iconInset),
				ImVec2(buttonPos.x + buttonSize - iconInset, buttonPos.y + buttonSize - iconInset));
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reset");
		}

		if (clicked)
		{
			EmitValueChanged(Variant(static_cast<Object*>(nullptr)));
		}
	}

	DrawRect RenderAssignButton(ImVec2 buttonSize, Resource const* resource)
	{
		std::string const buttonId   = m_labelId + "_resource";
		bool              isMatching = IsDraggedResourceMatching();
		ImGui::InvisibleButton(buttonId.c_str(), buttonSize);

		ImVec2 const min     = ImGui::GetItemRectMin();
		ImVec2 const max     = ImGui::GetItemRectMax();
		bool const   hovered = ImGui::IsItemHovered();

		if (ImGui::BeginDragDropTarget())
		{
			if (isMatching)
			{
				EditorDragDrop& dragDrop = EditorNode::Get()->m_dragDrop;
				dragDrop.AllowDrop();
				ImGuiPayload const* payload = ImGui::AcceptDragDropPayload(EditorDragDrop::PayloadType);
				if (payload != nullptr)
				{
					VirtualPath virtualPath;
					if (dragDrop.TryGetData(virtualPath))
					{
						Ref<Resource> res = ResourceLoader::Load(virtualPath);
						EmitValueChanged(Variant(res));
					}
				}
			}

			ImGui::EndDragDropTarget();
		}

		DrawAssignButton(min, max, hovered, resource);

		if (isMatching)
		{
			DrawDropPreviewOutline(min, max);
		}

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

		return DrawRect{ min, max };
	}

	void DrawAssignButton(ImVec2 min, ImVec2 max, bool hovered, Resource const* resource) const
	{
		ImDrawList*  drawList = ImGui::GetWindowDrawList();
		ImVec4 const color =
			hovered ? EditorUIStyle::ControlBackgroundHoveredColor() : EditorUIStyle::ControlBackgroundColor();
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
			ImTextureID const textureId = EditorIcons::GetIconId(m_info.m_hintData, "Node");
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

		return DrawRect{ min, max };
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
		if (payload != nullptr && payload->IsDataType(EditorDragDrop::PayloadType))
		{
			EditorNode* editorNode = EditorNode::Get();
			if (editorNode != nullptr)
			{
				VirtualPath draggedPath;
				if (editorNode->m_dragDrop.TryGetData(draggedPath))
				{
					Ref<Resource> const draggedResource = ResourceLoader::Load(draggedPath);
					if (draggedResource.IsValid()
						&& ClassDatabase::IsSubclassOf(draggedResource->GetClassName(), m_info.m_hintData))
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	std::string GetResourceDisplayName(Resource const& resource) const
	{
		VirtualPath const& sourcePath = resource.GetSourceFilePath();
		if (sourcePath.IsValid())
		{
			return sourcePath.GetFileName();
		}

		VirtualPath const& virtualPath = resource.GetVirtualPath();
		if (virtualPath.IsValid())
		{
			return virtualPath.GetFileName();
		}

		return resource.GetName();
	}

	void SetResourceTooltip(Resource const& resource) const
	{
		VirtualPath const& sourcePath  = resource.GetSourceFilePath();
		VirtualPath const& virtualPath = resource.GetVirtualPath();

		if (sourcePath.IsValid() && virtualPath.IsValid() && sourcePath != virtualPath)
		{
			ImGui::SetTooltip(
				"Source: %s\nResource: %s\nType: %s",
				sourcePath.CStr(),
				virtualPath.CStr(),
				m_info.m_hintData.c_str());
		}
		else if (sourcePath.IsValid())
		{
			ImGui::SetTooltip("Source: %s\nType: %s", sourcePath.CStr(), m_info.m_hintData.c_str());
		}
		else if (virtualPath.IsValid())
		{
			ImGui::SetTooltip("Resource: %s\nType: %s", virtualPath.CStr(), m_info.m_hintData.c_str());
		}
		else
		{
			ImGui::SetTooltip("Type: %s", m_info.m_hintData.c_str());
		}
	}
};
