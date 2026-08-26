#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/EngineService/EngineService.hpp"
#include "MingEngine/EngineService/RenderService.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/VisualInstance3D.hpp"
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

void Viewport::RegisterVisualizeInstance(VisualInstance3D* visualizeInstance)
{
	GUARANTEE_OR_DIE(visualizeInstance != nullptr, "Viewport::RegisterVisualizeInstance failed: instance is null.");

	SceneTree*     sceneTree  = GetSceneTree();
	ObjectID const instanceID = visualizeInstance->GetObjectID();
	if (sceneTree == nullptr || !instanceID.IsValid() || visualizeInstance->GetSceneTree() != sceneTree)
	{
		return;
	}

	if (std::find(m_instanceIDs.begin(), m_instanceIDs.end(), instanceID) == m_instanceIDs.end())
	{
		m_instanceIDs.push_back(instanceID);
	}
}

void Viewport::UnregisterVisualizeInstance(VisualInstance3D* visualizeInstance)
{
	GUARANTEE_OR_DIE(visualizeInstance != nullptr, "Viewport::UnregisterVisualizeInstance failed: instance is null.");

	ObjectID const instanceID = visualizeInstance->GetObjectID();
	if (instanceID.IsValid())
	{
		auto const foundInstance = std::find(m_instanceIDs.begin(), m_instanceIDs.end(), instanceID);
		if (foundInstance != m_instanceIDs.end())
		{
			m_instanceIDs.erase(foundInstance);
		}
	}
}

void Viewport::RegisterWorldCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	SceneTree*     sceneTree = GetSceneTree();
	ObjectID const cameraID  = camera->GetObjectID();
	if (sceneTree == nullptr || !cameraID.IsValid() || camera->GetSceneTree() != sceneTree)
	{
		return;
	}

	if (std::find(m_worldCameraIDs.begin(), m_worldCameraIDs.end(), cameraID) == m_worldCameraIDs.end())
	{
		m_worldCameraIDs.push_back(cameraID);

		if (m_worldCameraID == ObjectID::Invalid)
		{
			m_worldCameraID = cameraID;
		}
	}
}

void Viewport::UnregisterWorldCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	ObjectID const cameraID = camera->GetObjectID();
	auto const foundCamera  = std::find(m_worldCameraIDs.begin(), m_worldCameraIDs.end(), cameraID);
	if (foundCamera != m_worldCameraIDs.end())
	{
		m_worldCameraIDs.erase(foundCamera);

		if (m_worldCameraID == cameraID)
		{
			m_worldCameraID = m_worldCameraIDs.empty() ? ObjectID::Invalid : m_worldCameraIDs.front();
		}
	}
}

Camera3D* Viewport::GetWorldCamera() const
{
	SceneTree* sceneTree = GetSceneTree();
	if (sceneTree == nullptr)
	{
		return nullptr;
	}

	Camera3D*  camera    = ObjectDatabase::GetInstance<Camera3D>(m_worldCameraID);
	return camera != nullptr && camera->GetSceneTree() == sceneTree ? camera : nullptr;
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
	m_viewportInfo.m_outputRect       = AABB2(Vector2::Zero, (Vector2)dimensions);

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

	// 1) CameraContext pointers are transient because referenced nodes may be destroyed.
	float     aspect             = m_viewportInfo.m_outputResolution.x / (float)m_viewportInfo.m_outputResolution.y;
	Camera3D* worldCamera        = GetWorldCamera();
	m_viewportInfo.m_worldCamera = nullptr;
	if (worldCamera != nullptr)
	{
		m_tmpWorldCamera             = worldCamera->GetCameraContext(aspect);
		m_viewportInfo.m_worldCamera = &m_tmpWorldCamera;
	}

	// 2) Requests describe only the current frame.
	for (auto& requests : m_viewportInfo.m_renderRequests)
	{
		requests.clear();
	}

	// 3) Resolve object IDs and prune stale registrations while collecting data.
	for (auto instanceIter = m_instanceIDs.begin(); instanceIter != m_instanceIDs.end();)
	{
		auto* instance = ObjectDatabase::GetInstance<VisualInstance3D>(*instanceIter);
		if (instance == nullptr || instance->GetSceneTree() != sceneTree)
		{
			instanceIter = m_instanceIDs.erase(instanceIter);
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
}

ViewportInfo& Viewport::GetViewportInfo() { return m_viewportInfo; }

ViewportInfo const& Viewport::GetViewportInfo() const { return m_viewportInfo; }
