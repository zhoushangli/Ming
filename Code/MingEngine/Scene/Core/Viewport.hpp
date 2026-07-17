#pragma once

#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

class Camera3D;
class VisualizeInstance3D;

class Viewport : public Node
{
	MCLASS(Viewport, Node);

public:
	Viewport();
	~Viewport() override;

	// Scene objects register by NodeHandle so deferred destruction and reparenting
	// cannot leave raw instance or light pointers in the Viewport.
	void RegisterVisualizeInstance(VisualizeInstance3D* visualizeInstance);
	void UnregisterVisualizeInstance(VisualizeInstance3D* visualizeInstance);

	void      RegisterWorldCamera(Camera3D* camera);
	void      UnregisterWorldCamera(Camera3D* camera);
	Camera3D* GetWorldCamera() const;

	void    SetOutputResolution(IntVec2 dimensions);
	IntVec2 GetOutputResolution() const;

	// Per-frame preparation:
	// 1) Resolve cameras and update their projection.
	// 2) Clear transient request arrays.
	// 3) Resolve registered handles and collect current render data.
	void PrepareRenderData();

	ViewportInfo&       GetViewportInfo();
	ViewportInfo const& GetViewportInfo() const;

protected:
	void OnNotification(int notification);

private:
	std::vector<NodeHandle> m_instances;

	std::vector<NodeHandle> m_worldCameraInstances;
	NodeHandle              m_worldCameraHandle = NodeHandle::Invalid;
	CameraContext           m_tmpWorldCamera;

	ViewportInfo m_viewportInfo;
};
