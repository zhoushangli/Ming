#include "MingEngine/Editor/UI/ViewportPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"

#include "ThirdParty/imgui/imgui.h"

using namespace Math;

ViewportPanel::ViewportPanel() : EditorPanel("Viewport") {}

void ViewportPanel::OnRender(EditorUIContext& context)
{
	EditorNode const* editorNode = EditorNode::Get();
	std::string       sceneName  = editorNode != nullptr ? editorNode->GetCurrentSceneName() : std::string();
	std::string       title      = sceneName.empty() ? "[empty]" : sceneName;
	if (editorNode != nullptr && editorNode->IsSceneDirty())
	{
		title += "(*)";
	}
	title += "###Viewport";
	ImGui::Begin(title.c_str(), GetOpenState());
	ImGui::Button("Select");
	ImGui::SameLine();
	ImGui::Button("Move");
	ImGui::SameLine();
	ImGui::Button("Rotate");
	ImGui::SameLine();
	ImGui::Button("Scale");
	ImGui::SameLine();
	ImGui::Checkbox("Snap", &m_snap);
	ImGui::Separator();

	// Resize before reading the output texture so this frame uses the current viewport dimensions.
	ImVec2 const  availableSize = ImGui::GetContentRegionAvail();
	int           width         = Max(static_cast<int>(availableSize.x), 1);
	int           height        = Max(static_cast<int>(availableSize.y), 1);
	IntVec2 const panelDimensions(width, height);

	Viewport* viewport =
		context.m_sceneTree != nullptr ? dynamic_cast<Viewport*>(context.m_sceneTree->GetRoot()) : nullptr;
	if (viewport != nullptr && viewport->GetOutputResolution() != panelDimensions)
	{
		// The Viewport keeps its own output resolution, the panel only requests the size.
		viewport->SetResolution(panelDimensions);
	}

	if (viewport != nullptr)
	{
		GPUTexture* viewportTexture = g_engine->m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
		if (viewportTexture != nullptr)
		{
			ImTextureID  textureId  = g_engine->m_renderServer->GetImGuiTextureID(viewportTexture);
			ImTextureRef textureRef = ImTextureRef(textureId);
			ImGui::Image(textureRef, availableSize);

			// 1) Record viewport rect for mouse-to-world ray mapping
			// 2) Updated every frame to reflect dock/resize changes
			ImVec2 const imgMin = ImGui::GetItemRectMin();
			if (context.m_editorUI != nullptr)
			{
				context.m_editorUI->SetViewportRect(
					Vector2(imgMin.x, imgMin.y),
					Vector2(availableSize.x, availableSize.y));
			}
		}
	}
	context.m_isViewportImageHovered = ImGui::IsItemHovered();

	ImGui::End();
}
