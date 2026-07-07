#pragma once

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Editor/Gizmos/GizmosShapes.hpp"

#include <array>

class Camera3D;

// yz are used to determine the position of the axis circle
// x is used to determine the depth of the axis circle
struct Axis2D
{
	Vec2  m_yz;
	float m_x;
	int   m_axis; // 0 = x, 1 = y, 2 = z, 3 = -x, 4 = -y, 5 = -z
	bool  m_isPositive;
};

class ViewportAxisIndicator : public EditorGizmoVisual3D
{
	MCLASS(ViewportAxisIndicator, EditorGizmoVisual3D)

public:
	ViewportAxisIndicator();
	~ViewportAxisIndicator() override;

protected:
	void          OnNotification(int notification);
	RenderRequest SubmitRenderRequest() const override;

private:
	void RebuildVertexBuffer();

private:
	EulerAngles           m_lastCameraRotation = EulerAngles::Zero;
	Vec2                  m_center             = Vec2(0.f, 0.f);
	std::array<Axis2D, 6> m_axises;
};
