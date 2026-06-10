#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/EngineService/RenderService.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Window/Window.hpp"

#include <algorithm>

Viewport::Viewport()
{
	IntVec2 defaultResolution =
		g_engine->m_window != nullptr ? g_engine->m_window->GetClientDimensions() : IntVec2(1280, 720);
	SetOutputResolution(defaultResolution);
}

Viewport::~Viewport()
{
	// Viewport GPU resources must be destroyed while Engine and Renderer still exist.
	g_engine->m_renderer->DestroyViewportResources(m_viewportInfo);
}

void Viewport::OnEnterTree()
{
	// Node propagation assigns SceneTree, NodeHandle, and owning Viewport
	// before this callback registers the root Viewport for rendering.
	if (g_engineService != nullptr && g_engineService->m_renderService != nullptr)
	{
		g_engineService->m_renderService->RegisterViewport(this);
	}
}

void Viewport::OnExitTree()
{
	// Unregister before SceneTree clears handles and before the Viewport is deleted.
	if (g_engineService != nullptr && g_engineService->m_renderService != nullptr)
	{
		g_engineService->m_renderService->UnregisterViewport(this);
	}
}

void Viewport::RegisterVisualizeInstance(VisualizeInstance3D* visualizeInstance)
{
	GUARANTEE_OR_DIE(visualizeInstance != nullptr, "Viewport::RegisterVisualizeInstance failed: instance is null.");

	NodeHandle handle = visualizeInstance->GetHandle();
	if (!handle.IsValid())
	{
		return;
	}

	if (std::find(m_instances.begin(), m_instances.end(), handle) == m_instances.end())
	{
		m_instances.push_back(handle);
	}
}

void Viewport::UnregisterVisualizeInstance(VisualizeInstance3D* visualizeInstance)
{
	GUARANTEE_OR_DIE(visualizeInstance != nullptr, "Viewport::UnregisterVisualizeInstance failed: instance is null.");

	NodeHandle handle = visualizeInstance->GetHandle();
	if (handle.IsValid())
	{
		auto const foundInstance = std::find(m_instances.begin(), m_instances.end(), handle);
		if (foundInstance != m_instances.end())
		{
			m_instances.erase(foundInstance);
		}
	}
}

void Viewport::RegisterLight(Light3D* light)
{
	if (light == nullptr || !light->GetHandle().IsValid())
	{
		return;
	}

	NodeHandle handle = light->GetHandle();
	if (std::find(m_lights.begin(), m_lights.end(), handle) == m_lights.end())
	{
		m_lights.push_back(handle);
	}
}

void Viewport::UnregisterLight(Light3D* light)
{
	if (light == nullptr)
	{
		return;
	}

	auto const foundLight = std::find(m_lights.begin(), m_lights.end(), light->GetHandle());
	if (foundLight != m_lights.end())
	{
		m_lights.erase(foundLight);
	}
}

void Viewport::SetWorldCamera(Camera3D* camera)
{
	m_worldCameraHandle = camera != nullptr ? camera->GetHandle() : NodeHandle::Invalid;
}

void Viewport::SetUICamera(Camera3D* camera)
{
	m_uiCameraHandle = camera != nullptr ? camera->GetHandle() : NodeHandle::Invalid;
}

Camera3D* Viewport::GetWorldCamera() const
{
	SceneTree* sceneTree = GetSceneTree();
	return sceneTree != nullptr ? dynamic_cast<Camera3D*>(sceneTree->ResolveNode(m_worldCameraHandle)) : nullptr;
}

Camera3D* Viewport::GetUICamera() const
{
	SceneTree* sceneTree = GetSceneTree();
	return sceneTree != nullptr ? dynamic_cast<Camera3D*>(sceneTree->ResolveNode(m_uiCameraHandle)) : nullptr;
}

void Viewport::SetOutputResolution(IntVec2 dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	if (m_viewportInfo.m_outputResolution == dimensions)
	{
		return;
	}

	m_viewportInfo.m_outputResolution = dimensions;
	m_viewportInfo.m_outputRect       = AABB2(Vec2::Zero, (Vec2)dimensions);

	g_engine->m_renderer->ResizeViewport(m_viewportInfo, dimensions);
}

void Viewport::PrepareRenderData()
{
	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr)
	{
		return;
	}

	if (m_viewportInfo.m_outputResolution == IntVec2::Zero && g_engine->m_window != nullptr)
	{
		// The root Viewport defaults to the window size until an editor panel or
		// another owner explicitly requests a different output resolution.
		SetOutputResolution(g_engine->m_window->GetClientDimensions());
	}

	// 1) CameraContext pointers are transient because NodeHandles may change after reparenting.
	float aspect                 = m_viewportInfo.m_outputResolution.x / (float)m_viewportInfo.m_outputResolution.y;
	Camera3D* worldCamera        = GetWorldCamera();
	Camera3D* uiCamera           = GetUICamera();
	m_viewportInfo.m_worldCamera = nullptr;
	m_viewportInfo.m_uiCamera    = nullptr;
	if (worldCamera != nullptr)
	{
		m_tmpWorldCamera             = worldCamera->GetCamera(aspect);
		m_viewportInfo.m_worldCamera = &m_tmpWorldCamera;
	}
	if (uiCamera != nullptr)
	{
		m_tmpUICamera             = uiCamera->GetCamera(aspect);
		m_viewportInfo.m_uiCamera = &m_tmpUICamera;
	}

	// 2) Requests and lights describe only the current frame.
	for (auto& requests : m_viewportInfo.m_renderRequests)
	{
		requests.clear();
	}
	m_viewportInfo.m_lights.clear();

	// 3) Resolve handles and prune stale registrations while collecting data.
	for (auto instanceIter = m_instances.begin(); instanceIter != m_instances.end();)
	{
		auto* instance = dynamic_cast<VisualizeInstance3D*>(sceneTree->ResolveNode(*instanceIter));
		if (instance == nullptr)
		{
			instanceIter = m_instances.erase(instanceIter);
			continue;
		}

		if (instance->GetVisible())
		{
			RenderRequest request = instance->SubmitRenderRequest();
			if (request.IsValid())
			{
				m_viewportInfo.m_renderRequests[(size_t)request.m_pass].push_back(request);
			}
		}

		++instanceIter;
	}

	for (auto lightIter = m_lights.begin(); lightIter != m_lights.end();)
	{
		auto* light = dynamic_cast<Light3D*>(sceneTree->ResolveNode(*lightIter));
		if (light == nullptr)
		{
			lightIter = m_lights.erase(lightIter);
			continue;
		}

		m_viewportInfo.m_lights.push_back(light->GetLightInfo());
		++lightIter;
	}
}

ViewportInfo& Viewport::GetViewportInfo() { return m_viewportInfo; }

ViewportInfo const& Viewport::GetViewportInfo() const { return m_viewportInfo; }
