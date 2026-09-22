#include "MingEngine/Scene/3D/VisualInstance3D.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

void VisualInstance3D::OnNotification(int notification)
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr)
	{
		return;
	}

	switch (notification)
	{
	case Notification_EnterTree:
	{
		// 1) Create the instance once the owning Viewport is known.
		// 2) Join the Viewport Scenario so the RenderServer can draw this node.
		if (m_data.m_viewport != nullptr)
		{
			server->InstanceSetScenario(m_instanceRID, m_data.m_viewport->GetScenarioRID());
			server->InstanceSetTransform(m_instanceRID, GetWorldTransform());
			server->InstanceSetVisible(m_instanceRID, m_isVisible);
		}
		break;
	}
	case Notification_TransformChanged:
	{
		if (m_instanceRID.IsValid())
		{
			server->InstanceSetTransform(m_instanceRID, GetWorldTransform());
		}
		break;
	}
	}
}

VisualInstance3D::VisualInstance3D() { m_instanceRID = g_engine->m_renderServer->InstanceCreate(); }

VisualInstance3D::~VisualInstance3D()
{
	if (m_instanceRID.IsValid())
	{
		g_engine->m_renderServer->InstanceFree(m_instanceRID);
		m_instanceRID = RID::Invalid;
	}
}

void VisualInstance3D::SetVisible(bool visible)
{
	m_isVisible = visible;

	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server != nullptr && m_instanceRID.IsValid())
	{
		server->InstanceSetVisible(m_instanceRID, visible);
	}
}

bool VisualInstance3D::GetVisible() const { return m_isVisible; }
