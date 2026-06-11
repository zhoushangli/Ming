#include "MingEngine/Editor/Gizmos/ViewportAxisIndicator.hpp"

#include "MingEngine/Editor/EditorController.hpp"
#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include <algorithm>

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);

float constexpr kCircleRadius = 10.f;
float constexpr kLineWidth    = 3.f;
float constexpr kLineLength   = 50.f;
int constexpr kCircleSegments = 16;

void AddVertsForAxisLine(
	std::vector<Vertex>& verts, Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color)
{
	Vec2 const dir  = (end - start).GetNormalized();
	Vec2 const perp = Vec2(-dir.y, dir.x) * (thickness * 0.5f);

	verts.emplace_back(Vec3(start.x + perp.x, start.y + perp.y, 0.f), color);
	verts.emplace_back(Vec3(end.x + perp.x, end.y + perp.y, 0.f), color);
	verts.emplace_back(Vec3(end.x - perp.x, end.y - perp.y, 0.f), color);

	verts.emplace_back(Vec3(start.x + perp.x, start.y + perp.y, 0.f), color);
	verts.emplace_back(Vec3(end.x - perp.x, end.y - perp.y, 0.f), color);
	verts.emplace_back(Vec3(start.x - perp.x, start.y - perp.y, 0.f), color);
}

void AddVertsForAxisPoint(
	std::vector<Vertex>& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	for (int i = 0; i < kCircleSegments; ++i)
	{
		float const angle0 = (360.f / kCircleSegments) * (float)i;
		float const angle1 = (360.f / kCircleSegments) * (float)(i + 1);
		Vec2 const p0      = center + Vec2::MakeFromPolarDegrees(angle0, radius);
		Vec2 const p1      = center + Vec2::MakeFromPolarDegrees(angle1, radius);

		verts.emplace_back(Vec3(center.x, center.y, 0.f), color);
		verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
		verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
	}

	float const innerR = radius - thickness * 0.5f;
	float const outerR = radius + thickness * 0.5f;

	for (int i = 0; i < kCircleSegments; ++i)
	{
		float const angle0 = (360.f / kCircleSegments) * (float)i;
		float const angle1 = (360.f / kCircleSegments) * (float)(i + 1);
		Vec2 const inner0  = center + Vec2::MakeFromPolarDegrees(angle0, innerR);
		Vec2 const inner1  = center + Vec2::MakeFromPolarDegrees(angle1, innerR);
		Vec2 const outer0  = center + Vec2::MakeFromPolarDegrees(angle0, outerR);
		Vec2 const outer1  = center + Vec2::MakeFromPolarDegrees(angle1, outerR);

		verts.emplace_back(Vec3(outer0.x, outer0.y, 0.f), color);
		verts.emplace_back(Vec3(outer1.x, outer1.y, 0.f), color);
		verts.emplace_back(Vec3(inner1.x, inner1.y, 0.f), color);

		verts.emplace_back(Vec3(outer0.x, outer0.y, 0.f), color);
		verts.emplace_back(Vec3(inner1.x, inner1.y, 0.f), color);
		verts.emplace_back(Vec3(inner0.x, inner0.y, 0.f), color);
	}
}
} // namespace

ViewportAxisIndicator::ViewportAxisIndicator()
{
	SetReady(true);
	SetProcess(true);

	m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(std::vector<Vertex>{ Vertex(Vec3::Zero, Rgba8::White) });
}

ViewportAxisIndicator::~ViewportAxisIndicator() {}

void ViewportAxisIndicator::OnEnterTree()
{
	EditorGizmoVisual3D::OnEnterTree();

	Camera3D* editorCamera     = EditorController::Get()->GetCamera();
	EulerAngles cameraRotation = editorCamera->GetWorldOrientation();
	m_lastCameraRotation       = cameraRotation;
	RebuildVertexBuffer();
}

void ViewportAxisIndicator::OnProcess([[maybe_unused]] float deltaSeconds)
{
	Camera3D* editorCamera     = EditorController::Get()->GetCamera();
	EulerAngles cameraRotation = editorCamera->GetWorldOrientation();

	// We just hardcode the position
	IntVec2 dimensions = m_data.m_viewport->GetOutputResolution();
	Vec2 center        = (Vec2)dimensions - Vec2(100.f, 100.f);

	if (cameraRotation != m_lastCameraRotation || center != m_center)
	{
		m_lastCameraRotation = cameraRotation;
		m_center = center;
		RebuildVertexBuffer();
	}
}

RenderRequest ViewportAxisIndicator::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass           = RenderRequestPass::UI;
	request.m_modelToWorld   = Matrix4x4::Identity;
	request.m_vertexBuffer   = m_vertexBuffer;
	request.m_shader         = g_engine->m_renderer->CreateOrGetShader("Data/Shaders/DefaultUI");
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::DISABLED;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	return request;
}

void ViewportAxisIndicator::RebuildVertexBuffer()
{
	// Here we have two potential transform
	// 1) We need to get the inverse rotation of camera
	// this is because the rotMatrix is CameraToWorld, but we want WorldToCamera
	// 2) We need to get the CameraToScreen
	// because for 3D space, we use right hand coordinate system, when y is in the right of z
	// but for screen space, we use left hand coordinate system, when y is in the left of z

	m_verts.clear();
	Matrix4x4 rotMatrix = m_lastCameraRotation.GetAsMatrix_IFwd_JLeft_KUp();
	rotMatrix           = rotMatrix.GetOrthonormalInverse();

	Vec3 xAxis = rotMatrix.GetIBasis3D();
	Vec3 yAxis = rotMatrix.GetJBasis3D();
	Vec3 zAxis = rotMatrix.GetKBasis3D();

	xAxis.y = -xAxis.y;
	yAxis.y = -yAxis.y;
	zAxis.y = -zAxis.y;

	m_axises[0] = { Vec2(xAxis.y, xAxis.z), xAxis.x, 0, true };
	m_axises[1] = { Vec2(yAxis.y, yAxis.z), yAxis.x, 1, true };
	m_axises[2] = { Vec2(zAxis.y, zAxis.z), zAxis.x, 2, true };
	m_axises[3] = { Vec2(-xAxis.y, -xAxis.z), -xAxis.x, 3, false };
	m_axises[4] = { Vec2(-yAxis.y, -yAxis.z), -yAxis.x, 4, false };
	m_axises[5] = { Vec2(-zAxis.y, -zAxis.z), -zAxis.x, 5, false };

	std::sort(m_axises.begin(), m_axises.end(), [](Axis2D const& a, Axis2D const& b) { return a.m_x > b.m_x; });

	for (Axis2D const& axis : m_axises)
	{
		Rgba8 axisColor = Rgba8::White;
		if (axis.m_axis == 0 || axis.m_axis == 3)
			axisColor = kAxisXColor;
		else if (axis.m_axis == 1 || axis.m_axis == 4)
			axisColor = kAxisYColor;
		else if (axis.m_axis == 2 || axis.m_axis == 5)
			axisColor = kAxisZColor;

		axisColor.a = (unsigned char)RangeMap(axis.m_x, -1.f, 1.f, 255, 100);

		if (axis.m_isPositive)
		{
			AddVertsForAxisLine(m_verts, m_center, m_center + kLineLength * axis.m_yz, kLineWidth, axisColor);
		}

		AddVertsForAxisPoint(
			m_verts,
			m_center + (kLineLength + kCircleRadius) * axis.m_yz,
			kCircleRadius,
			kLineWidth,
			axisColor);
	}

	g_engine->m_renderer->UpdateVertexBuffer(m_vertexBuffer, m_verts);
}
