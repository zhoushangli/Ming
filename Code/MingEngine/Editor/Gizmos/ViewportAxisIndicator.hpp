#pragma once

#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"

class Camera3D;

class ViewportAxisIndicator : public EditorGizmoVisual3D
{
public:
	ViewportAxisIndicator();
	~ViewportAxisIndicator() override;

protected:
	void OnProcess(float deltaSeconds) override;
	RenderRequest SubmitRenderRequest() const override;

private:
	void RebuildVertexBuffer();

private:
	EulerAngles m_lastCameraRotation = EulerAngles::Zero;
};
