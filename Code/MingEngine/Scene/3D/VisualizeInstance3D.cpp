#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"

#include "MingEngine/Scene/Core/Viewport.hpp"

void VisualizeInstance3D::OnEnterTree()
{
	Node3D::OnEnterTree();
	// The owning Viewport is assigned by Node::PropagateEnterTree before this callback.
	if (m_data.m_viewport != nullptr)
	{
		m_data.m_viewport->RegisterVisualizeInstance(this);
	}
}

void VisualizeInstance3D::OnExitTree()
{
	// Unregister while both the old Viewport and this NodeHandle are still valid.
	if (m_data.m_viewport != nullptr)
	{
		m_data.m_viewport->UnregisterVisualizeInstance(this);
	}
}

void VisualizeInstance3D::SetVisible(bool visible) { m_isVisible = visible; }
bool VisualizeInstance3D::GetVisible() const { return m_isVisible; }
