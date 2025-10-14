#include "VertexUtils.hpp"
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

