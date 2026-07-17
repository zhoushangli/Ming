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
	m_lights.clear();
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

int RenderService::CreateLight(LightType type)
{
	LightEntry entry;
	entry.m_rid         = m_nextLightRid++;
	entry.m_info.m_type = type;
	m_lights.push_back(entry);
	return entry.m_rid;
}

void RenderService::FreeLight(int rid)
{
	auto const foundLight =
		std::find_if(m_lights.begin(), m_lights.end(), [rid](LightEntry const& entry) { return entry.m_rid == rid; });
	if (foundLight != m_lights.end())
	{
		m_lights.erase(foundLight);
	}
}

void RenderService::SetLightTransform(int rid, Matrix4x4 const& transform)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_transform = transform;
	}
}

void RenderService::SetLightColor(int rid, Color const& color)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_color = color;
	}
}

void RenderService::SetLightIntensity(int rid, float intensity)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_intensity = intensity;
	}
}

void RenderService::SetLightRange(int rid, float range)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_range = range;
	}
}

void RenderService::SetLightAttenuation(int rid, float attenuation)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_attenuation = attenuation;
	}
}

void RenderService::SetLightSpotAngle(int rid, float angle)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_spotAngle = angle;
	}
}

void RenderService::SetLightSpotAttenuation(int rid, float attenuation)
{
	LightEntry* light = FindLight(rid);
	if (light != nullptr)
	{
		light->m_info.m_spotAttenuation = attenuation;
	}
}

RenderService::LightEntry* RenderService::FindLight(int rid)
{
	auto const foundLight =
		std::find_if(m_lights.begin(), m_lights.end(), [rid](LightEntry const& entry) { return entry.m_rid == rid; });
	return foundLight != m_lights.end() ? &(*foundLight) : nullptr;
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
		viewportInfo.m_lights.clear();
		viewportInfo.m_lights.reserve(m_lights.size());
		for (LightEntry const& light : m_lights)
		{
			viewportInfo.m_lights.push_back(light.m_info);
		}
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
