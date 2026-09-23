#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Window/WindowSystem.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/Core/CanvasItem.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"

#include <algorithm>

namespace
{

void CollectCanvasItems(Node* node, Viewport* owner, RenderServer& server, float viewportHeight, RID viewportRID)
{
	// 1) Leave nested Viewports to their own collection pass
	if (node != owner && dynamic_cast<Viewport*>(node) != nullptr)
	{
		return;
	}

	// 2) Collect the parent before its children
	if (auto* item = dynamic_cast<CanvasItem*>(node))
	{
		RenderRequest request = item->BuildRenderRequest(server, viewportHeight);

		if (request.IsValid())
		{
			server.ViewportSubmitRenderRequest(viewportRID, request);
		}
	}

	// 3) Preserve child order
	for (Node* child : node->GetChildren())
	{
		CollectCanvasItems(child, owner, server, viewportHeight, viewportRID);
	}
}

} // namespace

Viewport::Viewport()
{
	IntVec2 defaultResolution =
		g_engine->m_windowSystem != nullptr ? g_engine->m_windowSystem->GetClientDimensions() : IntVec2(1280, 720);

	m_viewportRID      = g_engine->m_renderServer->ViewportCreate();
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

	ObjectID const cameraID    = camera->GetObjectID();
	auto const     foundCamera = std::find(m_worldCameraIDs.begin(), m_worldCameraIDs.end(), cameraID);
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

	Camera3D* camera = ObjectDatabase::GetInstance<Camera3D>(m_worldCameraID);
	return camera != nullptr && camera->GetSceneTree() == sceneTree ? camera : nullptr;
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

void Viewport::PrepareRenderData()
{
	if (GetSceneTree() == nullptr)
	{
		return;
	}

	if (m_outputResolution == IntVec2::Zero && g_engine->m_windowSystem != nullptr)
	{
		// The root Viewport defaults to the window size until an editor panel or
		// another owner explicitly requests a different output resolution.
		SetResolution(g_engine->m_windowSystem->GetClientDimensions());
	}

	RenderServer* server = g_engine->m_renderServer;

	// 1) The camera is bound to this Viewport by Camera3D on EnterTree / ExitTree.
	// 2) BeginFrame cleared this frame's request arrays, collection fills them again.
	// 3) Instances are drawn by the RenderServer through the Scenario instance list.
	server->ViewportBeginFrame(m_viewportRID);

	CollectCanvasItems(this, this, *server, static_cast<float>(m_outputResolution.y), m_viewportRID);
}
