#pragma once

#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

class Camera3D;
class VisualInstance3D;

class Viewport : public Node
{
	MCLASS(Viewport, Node);

public:
	Viewport();
	~Viewport() override;

	// Scene objects register by ObjectID so deferred destruction and reparenting
	// cannot leave raw instance or light pointers in the Viewport.
	void RegisterVisualizeInstance(VisualInstance3D *visualizeInstance);
	void UnregisterVisualizeInstance(VisualInstance3D *visualizeInstance);

	void RegisterWorldCamera(Camera3D *camera);
	void UnregisterWorldCamera(Camera3D *camera);
	Camera3D *GetWorldCamera() const;

	void SetOutputResolution(IntVec2 dimensions);
	IntVec2 GetOutputResolution() const;

	// Per-frame preparation:
	// 1) Resolve cameras and update their projection.
	// 2) Clear transient request arrays.
	// 3) Resolve registered handles and collect current render data.
	void PrepareRenderData();

	ViewportInfo &GetViewportInfo();
	ViewportInfo const &GetViewportInfo() const;

protected:
	void OnNotification(int notification);

private:
	std::vector<ObjectID> m_instanceIDs;

	std::vector<ObjectID> m_worldCameraIDs;
	ObjectID m_worldCameraID = ObjectID::Invalid;
	CameraContext m_tmpWorldCamera;

	ViewportInfo m_viewportInfo;
};
