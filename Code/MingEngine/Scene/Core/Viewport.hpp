#pragma once

#include "MingEngine/Scene/Core/Node.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

class Camera3D;
class Light3D;
class VisualizeInstance3D;

class Viewport : public Node
{
public:
	Viewport();
	~Viewport() override;

	// Scene objects register by NodeHandle so deferred destruction and reparenting
	// cannot leave raw instance or light pointers in the Viewport.
	void RegisterVisualizeInstance(VisualizeInstance3D* visualizeInstance);
	void UnregisterVisualizeInstance(VisualizeInstance3D* visualizeInstance);
	void RegisterLight(Light3D* light);
	void UnregisterLight(Light3D* light);

	void SetWorldCamera(Camera3D* camera);
	void SetUICamera(Camera3D* camera);
	Camera3D* GetWorldCamera() const;
	Camera3D* GetUICamera() const;

	void SetOutputResolution(IntVec2 dimensions);

	// Per-frame preparation:
	// 1) Resolve cameras and update their projection.
	// 2) Clear transient request/light arrays.
	// 3) Resolve registered handles and collect current render data.
	void PrepareRenderData();

	ViewportInfo&       GetViewportInfo();
	ViewportInfo const& GetViewportInfo() const;

protected:
	void OnEnterTree() override;
	void OnExitTree() override;

private:
	std::vector<NodeHandle> m_instances;
	std::vector<NodeHandle> m_lights;

	NodeHandle m_worldCameraHandle = NodeHandle::Invalid;
	NodeHandle m_uiCameraHandle    = NodeHandle::Invalid;

	// Transient data used during PrepareRenderData
	CameraContext m_tmpWorldCamera;
	CameraContext m_tmpUICamera;

	ViewportInfo m_viewportInfo;
};
