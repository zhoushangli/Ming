#include "VertexUtils.hpp"
#include <Engine/Math/MathUtils.hpp>

void TransformVertexArrayXY3D(int numVerts, Vertex* verts, float scaleXY, 
	float rotationDegreesAboutZ, Vec2 const& translationXY)
{
	for (int index = 0; index < numVerts; ++index)
	{
		Vec3& position = verts[index].m_position;
		TransformPositionXY3D(position, scaleXY, rotationDegreesAboutZ, translationXY);
	}
}

