#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

#include <set>

class Camera3D;
class Control;

// Form render side
// Viewport is basically a canvas, a paper, where the camera and its child node can render to
// It is a node with a renderable texture
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

	IntVec2 GetOutputResolution() const;
	void    UpdateResolution(IntVec2 dimensions);
	void    AddRootControl(Control* control);
	void    RemoveRootControl(Control* control);

	// All Viewport render state lives on the RenderServer and is addressed by this RID.
	RID GetViewportRID() const { return m_viewportRID; }
	RID GetCanvasLayerRID() const { return m_canvasLayerRID; }

	// Scenario that owns this Viewport's instances on the RenderServer.
	RID GetScenarioRID() const { return m_scenarioRID; }

protected:
	void OnNotification(int notification);

	void SetResolution(IntVec2 dimensions);

protected:
	Camera3D*           m_currentCamera = nullptr;
	std::set<Camera3D*> m_cameras;

	RID     m_viewportRID      = RID::Invalid;
	RID     m_canvasLayerRID   = RID::Invalid;
	RID     m_scenarioRID      = RID::Invalid;
	IntVec2 m_outputResolution = IntVec2::Zero;

	std::vector<Control*> m_rootControls;
};

// SubViewport is basically viewport and mainly used for off screen rendering
class SubViewport : public Viewport
{
	MCLASS(SubViewport, Viewport);

protected:
	static void BindMethods() {}
};
