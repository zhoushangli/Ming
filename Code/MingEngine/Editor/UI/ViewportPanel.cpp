#include "MingEngine/Editor/UI/ViewportPanel.hpp"

#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
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
	if (panelDimensions != m_dimensions)
	{
		m_dimensions = panelDimensions;
		EventArgs args;
		args.SetValue("width", std::to_string(width));
		args.SetValue("height", std::to_string(height));
		FireEvent("EditorViewportResized", args);
	}

	Viewport* viewport =
		context.m_sceneTree != nullptr ? dynamic_cast<Viewport*>(context.m_sceneTree->GetRoot()) : nullptr;
	if (viewport != nullptr)
	{
		GPUTexture* viewportTexture = viewport->GetViewportInfo().m_viewportOutputTexture;
		if (viewportTexture != nullptr)
		{
			ImTextureID  textureId  = (ImTextureID)(intptr_t)viewportTexture->GetShaderResourceView();
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
