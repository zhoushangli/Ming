#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"

class VisualInstance3D : public Node3D
{
	MCLASS(VisualInstance3D, Node3D);

public:
	VisualInstance3D();
	~VisualInstance3D() override;

	void SetVisible(bool visible);
	bool GetVisible() const;

protected:
	void        OnNotification(int notification);
	static void BindMethods() {};

public:
	bool m_isVisible = true;

protected:
	// Render instance that mirrors this node while it stays in the SceneTree.
	// e.g. InstanceSetScenario() links it to the Viewport Scenario so Render() can visit it
	RID m_instanceRID = RID::Invalid;
};
