#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include <algorithm>

Viewport::Viewport()
{
	IntVec2 defaultResolution =
		g_engine->m_windowSystem != nullptr ? g_engine->m_windowSystem->GetClientDimensions() : IntVec2(1280, 720);

	m_viewportRID      = g_engine->m_renderServer->ViewportCreate();
	m_canvasLayerRID   = g_engine->m_renderServer->CanvasLayerCreate();
	g_engine->m_renderServer->CanvasLayerSetViewport(m_canvasLayerRID, m_viewportRID);
	m_outputResolution = defaultResolution;
	g_engine->m_renderServer->ViewportSetResolution(m_viewportRID, m_outputResolution);

	// Every Viewport owns one Scenario so scene nodes have a target to register instances into.
	m_scenarioRID = g_engine->m_renderServer->ScenarioCreate();
	g_engine->m_renderServer->ViewportSetScenario(m_viewportRID, m_scenarioRID);
}

Viewport::~Viewport()
{
	// Viewport GPU resources must be destroyed while Engine and Renderer still exist.
	g_engine->m_renderServer->ScenarioFree(m_scenarioRID);
	g_engine->m_renderServer->CanvasLayerFree(m_canvasLayerRID);
	g_engine->m_renderServer->ViewportFree(m_viewportRID);
}

void Viewport::OnNotification(int notification)
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr)
	{
		return;
	}

	switch (notification)
	{
	case Notification_EnterTree:
		server->ViewportSetActive(m_viewportRID, true);
		break;
	case Notification_ExitTree:
		// Leaving the tree only deactivates: the RID and its size survive for re-entering.
		server->ViewportSetActive(m_viewportRID, false);
		break;
	}
}

void Viewport::AddCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	m_cameras.insert(camera);
}

void Viewport::RemoveCamera(Camera3D* camera)
{
	if (camera == nullptr)
	{
		return;
	}

	m_cameras.erase(camera);
	if (m_currentCamera == camera)
	{
		SetCurrentCamera(nullptr);
	}
}

void Viewport::SetCurrentCamera(Camera3D* camera)
{
	if (camera == m_currentCamera)
	{
		return;
	}

	m_currentCamera = camera;

	if (camera != nullptr)
	{
		g_engine->m_renderServer->ViewportSetCamera(m_viewportRID, camera->GetCameraRID());
	}
	else
	{
		g_engine->m_renderServer->ViewportSetCamera(m_viewportRID, RID::Invalid);
	}
}

Camera3D* Viewport::GetCurrentCamera() const { return m_currentCamera; }

void Viewport::ChangeToNextCamera()
{
	for (auto& it : m_cameras)
	{
		it->SetCurrent();
		return;
	}
}

IntVec2 Viewport::GetOutputResolution() const { return m_outputResolution; }

void Viewport::SetResolution(IntVec2 dimensions)
{
	if (dimensions.x <= 0 || dimensions.y <= 0)
	{
		return;
	}

	if (m_outputResolution == dimensions)
	{
		return;
	}

	m_outputResolution = dimensions;
	g_engine->m_renderServer->ViewportSetResolution(m_viewportRID, dimensions);
}
