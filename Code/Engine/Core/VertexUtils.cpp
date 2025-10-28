#include "Engine/Core/VertexUtils.hpp"

#include "Engine/Core/Vertex.hpp"
#include "Engine/Math/MathUtils.hpp"

void TransformVertexArrayXY3D(int numVerts, Vertex* verts, float scaleXY, 
	float rotationDegreesAboutZ, Vec2 const& translationXY)
{
	Vec2 iBasis = Vec2::MakeFromPolarDegrees(rotationDegreesAboutZ, scaleXY);
	Vec2 jBasis = iBasis.GetRotatedBy90Degrees();
	for (int index = 0; index < numVerts; ++index)
	{
		Vec3& position = verts[index].m_position;
		TransformPositionXY3D(position, iBasis, jBasis, translationXY);
	}
}

void AddVertsForAABB2D(std::vector<Vertex>& verts, const AABB2& bounds, const Rgba8& color)
{
	Vec3 pos0(bounds.m_mins.x, bounds.m_mins.y, 0.f);
	Vec3 pos1(bounds.m_maxs.x, bounds.m_mins.y, 0.f);
	Vec3 pos2(bounds.m_maxs.x, bounds.m_maxs.y, 0.f);
	Vec3 pos3(bounds.m_mins.x, bounds.m_maxs.y, 0.f);

	Vec2 uv0(0.f, 0.f);
	Vec2 uv1(1.f, 0.f);
	Vec2 uv2(1.f, 1.f);
	Vec2 uv3(0.f, 1.f);

    verts.emplace_back(pos0, color, uv0);
    verts.emplace_back(pos1, color, uv1);
    verts.emplace_back(pos2, color, uv2);

    verts.emplace_back(pos0, color, uv0);
    verts.emplace_back(pos2, color, uv2);
    verts.emplace_back(pos3, color, uv3);
}

