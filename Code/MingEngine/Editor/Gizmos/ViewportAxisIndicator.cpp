#include "MingEngine/Editor/Gizmos/ViewportAxisIndicator.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Editor/EditorController.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

namespace
{
Rgba8 const kAxisXColor(255, 70, 105, 255);
Rgba8 const kAxisYColor(155, 225, 20, 255);
Rgba8 const kAxisZColor(55, 160, 255, 255);
Rgba8 const kCircleBgColor(80, 80, 80, 120);

float constexpr kIndicatorRadius = 32.f;
float constexpr kCircleRadius    = 8.f;
float constexpr kNegCircleRadius = 5.f;
float constexpr kLineWidth       = 2.f;
int constexpr kCircleSegments    = 16;

void AddVertsForScreenLine2D(
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

void AddVertsForScreenDisc2D(std::vector<Vertex>& verts, Vec2 const& center, float radius, Rgba8 const& color)
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
}

void AddVertsForScreenRing2D(
	std::vector<Vertex>& verts, Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
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

ViewportAxisIndicator::ViewportAxisIndicator() {}

ViewportAxisIndicator::~ViewportAxisIndicator() {}

void ViewportAxisIndicator::OnProcess(float deltaSeconds)
{
	Camera3D* editorCamera = EditorController::Get()->GetCamera();
	EulerAngles cameraRotation = editorCamera->GetWorldOrientation();

	if (cameraRotation != m_lastCameraRotation)
	{
		m_lastCameraRotation = cameraRotation;
		RebuildVertexBuffer();
	}
}

RenderRequest ViewportAxisIndicator::SubmitRenderRequest() const
{
	RenderRequest request;
	request.m_pass           = RenderRequestPass::UI;
	request.m_modelToWorld   = Matrix4x4::Identity;
	request.m_vertexBuffer   = m_vertexBuffer;
	request.m_blendMode      = BlendMode::ALPHA;
	request.m_depthMode      = DepthMode::DISABLED;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_NONE;
	return request;
}

void ViewportAxisIndicator::RebuildVertexBuffer()
{
	m_verts.clear();
	Matrix4x4 rotMatrix = m_lastCameraRotation.GetAsMatrix_IFwd_JLeft_KUp();

	Vec3 xAxis = rotMatrix.GetIBasis3D();
	Vec3 yAxis = rotMatrix.GetJBasis3D();
	Vec3 zAxis = rotMatrix.GetKBasis3D();


}
