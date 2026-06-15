#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/RaycastUtils.hpp"

void TriangleMeshCollider3D::SetMesh(std::vector<Vertex> const& vertices)
{
	m_vertices = vertices;
}

GameRaycastResult TriangleMeshCollider3D::Raycast(RaycastInfo const& info) const
{
	GameRaycastResult bestResult;
	bestResult.m_rayStartPos  = info.m_startPos;
	bestResult.m_rayFwdNormal = info.m_forwardNormal;
	bestResult.m_rayMaxLength = info.m_maxLength;

	if (m_vertices.size() < 3)
	{
		return bestResult;
	}

	Matrix4x4       localToWorld = GetWorldTransform();
	Matrix4x4 const worldToLocal = localToWorld.GetOrthonormalInverse();
	Vec3 const      localStart   = worldToLocal.TransformPosition3D(info.m_startPos);
	Vec3 const      localForward = worldToLocal.TransformDirection3D(info.m_forwardNormal);

	for (size_t vertexIndex = 0; vertexIndex + 2 < m_vertices.size(); vertexIndex += 3)
	{
		RaycastResult3D const triangleResult = RaycastVsTriangle3D(
			localStart,
			localForward,
			info.m_maxLength,
			m_vertices[vertexIndex].m_position,
			m_vertices[vertexIndex + 1].m_position,
			m_vertices[vertexIndex + 2].m_position
		);

		if (!triangleResult.m_didImpact)
		{
			continue;
		}

		if (!bestResult.m_didImpact || triangleResult.m_impactDist < bestResult.m_impactDist)
		{
			(RaycastResult3D&)bestResult = triangleResult;
		}
	}

	if (bestResult.m_didImpact)
	{
		bestResult.m_impactPos     = localToWorld.TransformPosition3D(bestResult.m_impactPos);
		bestResult.m_impactNormal  = localToWorld.TransformDirection3D(bestResult.m_impactNormal);
		bestResult.m_hitNodeHandle = GetHandle();
	}

	bestResult.m_rayStartPos  = info.m_startPos;
	bestResult.m_rayFwdNormal = info.m_forwardNormal;
	bestResult.m_rayMaxLength = info.m_maxLength;

	return bestResult;
}

void TriangleMeshCollider3D::OnProcess([[maybe_unused]] float deltaSeconds) { }

