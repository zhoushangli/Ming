#include "MingEngine/EngineService/RenderService.hpp"

#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

#include <algorithm>

void RenderService::Startup()
{
	RegisterEvent("WindowResized", RenderService::OnWindowResized);
	RegisterEvent("EditorViewportResized", RenderService::OnEditorViewportResized);
}

void RenderService::Shutdown()
{
	UnregisterEvent("EditorViewportResized", RenderService::OnEditorViewportResized);
	UnregisterEvent("WindowResized", RenderService::OnWindowResized);

	m_viewports.clear();
}

void RenderService::RegisterViewport(Viewport* viewport)
{
	if (viewport == nullptr || std::find(m_viewports.begin(), m_viewports.end(), viewport) != m_viewports.end())
	{
		return;
	}

	m_viewports.push_back(viewport);
}

void RenderService::UnregisterViewport(Viewport* viewport)
{
	auto const foundViewport = std::find(m_viewports.begin(), m_viewports.end(), viewport);
	if (foundViewport != m_viewports.end())
	{
		m_viewports.erase(foundViewport);
	}
}

void RenderService::Render() const
{
	Viewport* presentedViewport = nullptr;
	for (Viewport* viewport : m_viewports)
	{
		if (viewport == nullptr)
		{
			continue;
		}

		// 1) Rebuild transient render data from the Viewport's SceneTree.
		// 2) Render into this Viewport's own render targets.
		viewport->PrepareRenderData();
		ViewportInfo& viewportInfo = viewport->GetViewportInfo();
		g_engine->m_renderer->ClearSceneTargets(viewportInfo);
		g_engine->m_renderer->SetViewport(viewportInfo.m_outputRect.GetDimensions(), viewportInfo.m_outputRect.m_mins);
		g_engine->m_renderer->RenderViewport(viewportInfo);
		if (presentedViewport == nullptr)
		{
			presentedViewport = viewport;
		}
	}

	if (presentedViewport != nullptr)
	{
		// 3) The first registered Viewport is the main output for now.
		// Future editor Viewports still render to textures without replacing the back buffer.
		ViewportInfo const& viewportInfo = presentedViewport->GetViewportInfo();
		g_engine->m_renderer->CopyTextureToBackBuffer(viewportInfo.m_viewportOutputTexture);
	}
}

bool RenderService::OnWindowResized(EventArgs& args)
{
	int     width  = args.GetValue("width", 0);
	int     height = args.GetValue("height", 0);
	IntVec2 newDimensions(width, height);

	g_engine->m_renderer->ResizeBackBuffer(newDimensions);

	return false;
}

bool RenderService::OnEditorViewportResized(EventArgs& args)
{
	int     width  = args.GetValue("width", 0);
	int     height = args.GetValue("height", 0);
	IntVec2 newDimensions(width, height);

	for (Viewport* viewport : g_engineService->m_renderService->m_viewports)
	{
		if (viewport != nullptr)
		{
			viewport->SetOutputResolution(newDimensions);
		}
	}

	return false;
}
