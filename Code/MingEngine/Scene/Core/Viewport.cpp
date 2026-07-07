#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/EngineService/RenderService.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"

#include <algorithm>

Viewport::Viewport()
{
	IntVec2 defaultResolution =
		g_engine->m_windowSystem != nullptr ? g_engine->m_windowSystem->GetClientDimensions() : IntVec2(1280, 720);
	SetOutputResolution(defaultResolution);
}

Viewport::~Viewport()
{
	// Viewport GPU resources must be destroyed while Engine and Renderer still exist.
	g_engine->m_renderer->DestroyViewportResources(m_viewportInfo);
}

void Viewport::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
		if (g_engineService != nullptr && g_engineService->m_renderService != nullptr)
		{
			g_engineService->m_renderService->RegisterViewport(this);
		}
		break;
	case NotificationType::ExitTree:
		if (g_engineService != nullptr && g_engineService->m_renderService != nullptr)
		{
			g_engineService->m_renderService->UnregisterViewport(this);
		}
		break;
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

void Viewport::RegisterWorldCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	NodeHandle handle = camera->GetHandle();
	if (!handle.IsValid())
	{
		return;
	}

	if (std::find(m_worldCameraInstances.begin(), m_worldCameraInstances.end(), handle) == m_worldCameraInstances.end())
	{
		m_worldCameraInstances.push_back(handle);

		if (m_worldCameraHandle == NodeHandle::Invalid)
		{
			m_worldCameraHandle = handle;
		}
	}
}

void Viewport::UnregisterWorldCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	NodeHandle handle      = camera->GetHandle();
	auto const foundCamera = std::find(m_worldCameraInstances.begin(), m_worldCameraInstances.end(), handle);
	if (foundCamera != m_worldCameraInstances.end())
	{
		m_worldCameraInstances.erase(foundCamera);

		if (m_worldCameraHandle == handle)
		{
			m_worldCameraHandle = m_worldCameraInstances.empty() ? NodeHandle::Invalid : m_worldCameraInstances.front();
		}
	}
}

Camera3D* Viewport::GetWorldCamera() const
{
	SceneTree* sceneTree = GetSceneTree();
	Camera3D*  camera    = dynamic_cast<Camera3D*>(sceneTree->ResolveNode(m_worldCameraHandle));
	return camera;
}

IntVec2 Viewport::GetOutputResolution() const { return m_viewportInfo.m_outputResolution; }

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

	if (m_viewportInfo.m_outputResolution == IntVec2::Zero && g_engine->m_windowSystem != nullptr)
	{
		// The root Viewport defaults to the window size until an editor panel or
		// another owner explicitly requests a different output resolution.
		SetOutputResolution(g_engine->m_windowSystem->GetClientDimensions());
	}

	// 1) CameraContext pointers are transient because NodeHandles may change after reparenting.
	float     aspect             = m_viewportInfo.m_outputResolution.x / (float)m_viewportInfo.m_outputResolution.y;
	Camera3D* worldCamera        = GetWorldCamera();
	m_viewportInfo.m_worldCamera = nullptr;
	if (worldCamera != nullptr)
	{
		m_tmpWorldCamera             = worldCamera->GetCameraContext(aspect);
		m_viewportInfo.m_worldCamera = &m_tmpWorldCamera;
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
