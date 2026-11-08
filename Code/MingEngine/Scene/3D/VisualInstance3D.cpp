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
		// 1) Use the instance created with this node.
		// 2) Join the Viewport Scenario so the RenderServer can draw this node.
		if (m_data.m_viewport != nullptr)
		{
			server->Instance3DSetScenario(m_instanceRID, m_data.m_viewport->GetScenarioRID());
			server->Instance3DSetTransform(m_instanceRID, GetWorldTransform());
			server->Instance3DSetVisible(m_instanceRID, IsVisibleInHierarchy());
		}

		break;
	}

	case Notification_ExitTree:
	{
		server->Instance3DSetScenario(m_instanceRID, RID::Invalid);
		break;
	}

	case Notification_TransformChanged:
	{
		if (m_instanceRID.IsValid())
		{
			server->Instance3DSetTransform(m_instanceRID, GetWorldTransform());
		}

		break;
	}

	case Notification_VisibilityChanged:
	{
		server->Instance3DSetVisible(m_instanceRID, IsVisibleInHierarchy());
		break;
	}
	}
}

VisualInstance3D::VisualInstance3D()
{
	m_instanceRID = g_engine->m_renderServer->Instance3DCreate();
}

VisualInstance3D::~VisualInstance3D()
{
	if (m_instanceRID.IsValid())
	{
		g_engine->m_renderServer->Instance3DFree(m_instanceRID);
		m_instanceRID = RID::Invalid;
	}
}
