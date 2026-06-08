#include "MingEngine/Editor/UI/ViewportPanel.hpp"

#include "MingEngine/Editor/UI/EditorUIContext.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Event/EventSystem.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/Texture.hpp"

#include "ThirdParty/imgui/imgui.h"

ViewportPanel::ViewportPanel()
	: m_panel("Viewport")
{
}

void ViewportPanel::Render(EditorUIContext& context)
{
	if (!m_panel.IsOpen())
	{
		return;
	}

	ImGui::Begin(m_panel.GetTitle(), m_panel.GetOpenState());
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
	ImVec2 const availableSize = ImGui::GetContentRegionAvail();
	int          width         = Max(static_cast<int>(availableSize.x), 1);
	int          height        = Max(static_cast<int>(availableSize.y), 1);
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
		Texture* viewportTexture = viewport->GetViewportInfo().m_viewportOutputTexture;
		if (viewportTexture != nullptr)
		{
			ImTextureID  textureId  = (ImTextureID)(intptr_t)viewportTexture->GetShaderResourceView();
			ImTextureRef textureRef = ImTextureRef(textureId);
			ImGui::Image(textureRef, availableSize);
		}
	}
	ImGui::End();
}

EditorPanel& ViewportPanel::GetPanel() { return m_panel; }

EditorPanel const& ViewportPanel::GetPanel() const { return m_panel; }
