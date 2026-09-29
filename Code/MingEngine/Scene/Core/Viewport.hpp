#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include <set>

class Camera3D;

class Viewport : public Node
{
	MCLASS(Viewport, Node);

public:
	Viewport();
	~Viewport() override;

	void      AddCamera(Camera3D* camera);
	void      RemoveCamera(Camera3D* camera);
	void      SetCurrentCamera(Camera3D* camera);
	Camera3D* GetCurrentCamera() const;
	void      ChangeToNextCamera();

	void    SetResolution(IntVec2 dimensions);
	IntVec2 GetOutputResolution() const;

	// All Viewport render state lives on the RenderServer and is addressed by this RID.
	RID GetViewportRID() const { return m_viewportRID; }
	RID GetCanvasLayerRID() const { return m_canvasLayerRID; }

	// Scenario that owns this Viewport's instances on the RenderServer.
	RID GetScenarioRID() const { return m_scenarioRID; }

protected:
	void OnNotification(int notification);

private:
	Camera3D*           m_currentCamera = nullptr;
	std::set<Camera3D*> m_cameras;

	RID     m_viewportRID      = RID::Invalid;
	RID     m_canvasLayerRID   = RID::Invalid;
	RID     m_scenarioRID      = RID::Invalid;
	IntVec2 m_outputResolution = IntVec2::Zero;
};
