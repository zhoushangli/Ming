#pragma once

#include "MingEngine/Core/Render/RID.hpp"
#include "MingEngine/Scene/Core/Node.hpp"

class Camera3D;

class Viewport : public Node
{
	MCLASS(Viewport, Node);

public:
	Viewport();
	~Viewport() override;

	void      RegisterWorldCamera(Camera3D* camera);
	void      UnregisterWorldCamera(Camera3D* camera);
	Camera3D* GetWorldCamera() const;

	void    SetResolution(IntVec2 dimensions);
	IntVec2 GetOutputResolution() const;

	// Per-frame preparation:
	// 1) Begin the frame on the RenderServer, which clears the transient request data.
	// 2) Resolve registered handles and submit current render requests through the RID.
	// Cameras are bound to the Viewport by Camera3D on EnterTree / ExitTree.
	void PrepareRenderData();

	// All Viewport render state lives on the RenderServer and is addressed by this RID.
	RID GetViewportRID() const { return m_viewportRID; }

	// Scenario that owns this Viewport's instances on the RenderServer.
	RID GetScenarioRID() const { return m_scenarioRID; }

protected:
	void OnNotification(int notification);

private:
	std::vector<ObjectID> m_worldCameraIDs;
	ObjectID              m_worldCameraID = ObjectID::Invalid;

	RID     m_viewportRID      = RID::Invalid;
	RID     m_scenarioRID      = RID::Invalid;
	IntVec2 m_outputResolution = IntVec2::Zero;
};
