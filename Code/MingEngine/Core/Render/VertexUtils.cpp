#include "MingEngine/Core/Render/VertexUtils.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"
#include "MingEngine/Core/Render/Vertex.hpp"

using namespace Math;

void TransformVertexArrayXY3D(
	int numVerts, Vertex* verts, float scaleXY, float rotationDegreesAboutZ, Vector2 const& translationXY)
{
	Vector2 iBasis = Vector2::MakeFromPolarDegrees(rotationDegreesAboutZ, scaleXY);
	Vector2 jBasis = iBasis.GetRotatedBy90Degrees();
	for (int index = 0; index < numVerts; ++index)
	{
		Vector3& position = verts[index].m_position;
		TransformPositionXY3D(position, iBasis, jBasis, translationXY);
	}
}

void TransformVertexArray3D(std::vector<Vertex>& verts, const Matrix4x4& transform)
{
	for (Vertex& vert : verts)
	{
		vert.m_position  = transform.TransformPosition3D(vert.m_position);
		vert.m_tangent   = transform.TransformVectorQuantity3D(vert.m_tangent);
		vert.m_bitangent = transform.TransformVectorQuantity3D(vert.m_bitangent);
		vert.m_normal    = transform.TransformVectorQuantity3D(vert.m_normal);
	}
}

AABB2 GetVertexBounds2D(const std::vector<Vertex>& verts)
{
	AABB2 bounds;

	for (const Vertex& vert : verts)
	{
		bounds.StretchToIncludePoint(Vector2(vert.m_position.x, vert.m_position.y));
	}

	return bounds;
}

AABB3 GetVertexBounds3D(Vertex const* vertices, size_t numVerts)
{
	if (numVerts == 0)
	{
		return AABB3::Zero;
	}

	AABB3 bounds(AABB3(vertices[0].m_position, vertices[0].m_position));
	for (size_t i = 1; i < numVerts; ++i)
	{
		bounds.StretchToIncludePoint(vertices[i].m_position);
	}

	return bounds;
}

AABB3 GetVertexBounds3D(std::vector<Vertex> const& vertices)
{
	return GetVertexBounds3D(vertices.data(), vertices.size());
}

void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Color color)
{
	AddVertsForAABB2D(verts, alignedBox, color, Vector2::Zero, Vector2::One);
}

void AddVertsForAABB2D(
	std::vector<Vertex>& verts, AABB2 const& alignedBox, Color color, Vector2 uvAtMins, Vector2 uvAtMaxs)
{
	Vector2 mins = alignedBox.m_mins;
	Vector2 maxs = alignedBox.m_maxs;

	verts.emplace_back(Vector3(mins.x, mins.y, 0.f), color, Vector2(uvAtMins.x, uvAtMins.y));
	verts.emplace_back(Vector3(maxs.x, mins.y, 0.f), color, Vector2(uvAtMaxs.x, uvAtMins.y));
	verts.emplace_back(Vector3(maxs.x, maxs.y, 0.f), color, Vector2(uvAtMaxs.x, uvAtMaxs.y));

	verts.emplace_back(Vector3(mins.x, mins.y, 0.f), color, Vector2(uvAtMins.x, uvAtMins.y));
	verts.emplace_back(Vector3(maxs.x, maxs.y, 0.f), color, Vector2(uvAtMaxs.x, uvAtMaxs.y));
	verts.emplace_back(Vector3(mins.x, maxs.y, 0.f), color, Vector2(uvAtMins.x, uvAtMaxs.y));
}

void AddVertsForDisc2D(std::vector<Vertex>& verts, Vector2 discCenter, float discRadius, Color color)
{
	int numSides = (int)RangeMapClamped(discRadius, 1.f, 10.f, 12.f, 48.f);

	float delta = 360.0f / (float)numSides;
	for (int i = 0; i < numSides; ++i)
	{
		float   angle0 = i * delta;
		float   angle1 = (i + 1) * delta;
		Vector2 p0     = discCenter;
		Vector2 p1     = discCenter + Vector2::MakeFromPolarDegrees(angle0, discRadius);
		Vector2 p2     = discCenter + Vector2::MakeFromPolarDegrees(angle1, discRadius);
		verts.emplace_back(Vector3(p0.x, p0.y, 0.f), color);
		verts.emplace_back(Vector3(p1.x, p1.y, 0.f), color);
		verts.emplace_back(Vector3(p2.x, p2.y, 0.f), color);
	}
}

void AddVertsForRing2D(std::vector<Vertex>& verts, Vector2 ringCenter, float ringRadius, float thickness, Color color)
{
	int numSides = (int)RangeMapClamped(ringRadius, 1.f, 10.f, 36.f, 360.f);

	float innerRadius = ringRadius - thickness * 0.5f;
	float outerRadius = ringRadius + thickness * 0.5f;

	float delta = 360.0f / (float)numSides;
	for (int i = 0; i < numSides; ++i)
	{
		float   angle0 = i * delta;
		float   angle1 = (i + 1) * delta;
		Vector2 inner0 = ringCenter + Vector2::MakeFromPolarDegrees(angle0, innerRadius);
		Vector2 inner1 = ringCenter + Vector2::MakeFromPolarDegrees(angle1, innerRadius);
		Vector2 outer0 = ringCenter + Vector2::MakeFromPolarDegrees(angle0, outerRadius);
		Vector2 outer1 = ringCenter + Vector2::MakeFromPolarDegrees(angle1, outerRadius);

		// First triangle
		verts.emplace_back(Vector3(inner0.x, inner0.y, 0.f), color);
		verts.emplace_back(Vector3(outer0.x, outer0.y, 0.f), color);
		verts.emplace_back(Vector3(outer1.x, outer1.y, 0.f), color);

		// Second triangle
		verts.emplace_back(Vector3(inner0.x, inner0.y, 0.f), color);
		verts.emplace_back(Vector3(outer1.x, outer1.y, 0.f), color);
		verts.emplace_back(Vector3(inner1.x, inner1.y, 0.f), color);
	}
}

void AddVertsForOBB2D(std::vector<Vertex>& verts, OBB2 const& orientedBox, Color color)
{
	Vector2 corners[4];
	orientedBox.GetCornerPoints(corners);

	// Two triangles
	verts.emplace_back(Vector3(corners[0].x, corners[0].y, 0.f), color);
	verts.emplace_back(Vector3(corners[1].x, corners[1].y, 0.f), color);
	verts.emplace_back(Vector3(corners[2].x, corners[2].y, 0.f), color);

	verts.emplace_back(Vector3(corners[0].x, corners[0].y, 0.f), color);
	verts.emplace_back(Vector3(corners[2].x, corners[2].y, 0.f), color);
	verts.emplace_back(Vector3(corners[3].x, corners[3].y, 0.f), color);
}

void AddVertsForSector2D(
	std::vector<Vertex>& verts,
	Vector2              sectorOrigin,
	float                sectorForwardDegrees,
	float                sectorApertureDegrees,
	float                sectorRadius,
	Color                color)
{
	// Clamp aperture to [0, 360]
	float aperture = GetClamped(sectorApertureDegrees, 0.f, 360.f);
	if (aperture <= 0.f || sectorRadius <= 0.f)
	{
		return;
	}

	// Determine number of sides based on aperture and radius (similar to disc)
	int sectorSides = (int)RangeMapClamped(sectorRadius, 1.f, 10.f, 36.f, 360.f);

	float startAngle = sectorForwardDegrees - aperture * 0.5f;
	float delta      = aperture / (float)sectorSides;

	for (int i = 0; i < sectorSides; ++i)
	{
		float   angle0 = startAngle + i * delta;
		float   angle1 = startAngle + (i + 1) * delta;
		Vector2 p0     = sectorOrigin;
		Vector2 p1     = sectorOrigin + Vector2::MakeFromPolarDegrees(angle0, sectorRadius);
		Vector2 p2     = sectorOrigin + Vector2::MakeFromPolarDegrees(angle1, sectorRadius);
		verts.emplace_back(Vector3(p0.x, p0.y, 0.f), color);
		verts.emplace_back(Vector3(p1.x, p1.y, 0.f), color);
		verts.emplace_back(Vector3(p2.x, p2.y, 0.f), color);
	}
}

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Vector2 boneStart, Vector2 boneEnd, float radius, Color color)
{
	// Draw the body as a rectangle using AddVertsForLineSegment2D
	Vector2 dir          = (boneEnd - boneStart).GetNormalized();
	Vector2 right        = dir.GetRotatedByMinus90Degrees();
	Vector2 thicknessVec = right * (radius * 2.f);

	OBB2 capsuleBody             = OBB2();
	capsuleBody.m_center         = (boneStart + boneEnd) * 0.5f;
	capsuleBody.m_iBasisNormal   = dir;
	capsuleBody.m_halfDimensions = Vector2((boneEnd - boneStart).GetLength() * 0.5f, radius);

	AddVertsForOBB2D(verts, capsuleBody, color);

	// Draw the semicircle at each end using AddVertsForSector2D
	float forwardDegrees = dir.GetOrientationDegrees();
	AddVertsForSector2D(verts, boneStart, forwardDegrees + 180.f, 180.f, radius, color);
	AddVertsForSector2D(verts, boneEnd, forwardDegrees, 180.f, radius, color);
}

void AddVertsForTriangle2D(std::vector<Vertex>& verts, Vector2 ccw0, Vector2 ccw1, Vector2 ccw2, Color color)
{
	verts.emplace_back(Vector3(ccw0.x, ccw0.y, 0.f), color);
	verts.emplace_back(Vector3(ccw1.x, ccw1.y, 0.f), color);
	verts.emplace_back(Vector3(ccw2.x, ccw2.y, 0.f), color);
}

void AddVertsForLineSegment2D(std::vector<Vertex>& verts, Vector2 start, Vector2 end, Vector2 thickness, Color color)
{
	Vector2 dir       = (end - start).GetNormalized();
	Vector2 right     = dir.GetRotatedByMinus90Degrees();
	Vector2 halfThick = right * (thickness.x * 0.5f);

	Vector2 p0 = start + halfThick;
	Vector2 p1 = end + halfThick;
	Vector2 p2 = end - halfThick;
	Vector2 p3 = start - halfThick;

	verts.emplace_back(Vector3(p0.x, p0.y, 0.f), color);
	verts.emplace_back(Vector3(p1.x, p1.y, 0.f), color);
	verts.emplace_back(Vector3(p2.x, p2.y, 0.f), color);

	verts.emplace_back(Vector3(p0.x, p0.y, 0.f), color);
	verts.emplace_back(Vector3(p2.x, p2.y, 0.f), color);
	verts.emplace_back(Vector3(p3.x, p3.y, 0.f), color);
}

void AddVertsForInfiniteLine2D(
	std::vector<Vertex>& verts, Vector2 pointOnLine, Vector2 anotherPointOnLine, float thickness, Color color)
{
	Vector2 direction = anotherPointOnLine - pointOnLine;
	AddVertsForLineSegment2D(
		verts,
		pointOnLine - direction.GetNormalized() * 10000.f,
		pointOnLine + direction.GetNormalized() * 10000.f,
		Vector2(thickness, thickness),
		color);
}

void AddVertsForArrow2D(
	std::vector<Vertex>& verts, Vector2 tailPos, Vector2 tipPos, float arrowSize, float lineThickness, Color color)
{
	Vector2 dir    = (tipPos - tailPos).GetNormalized();
	float   length = (tipPos - tailPos).GetLength();

	float headLength  = (arrowSize > 0.f) ? std::min(arrowSize, length * 0.5f) : length * 0.2f;
	float shaftLength = length - headLength;

	Vector2 headBase = tipPos - dir * headLength;

	Vector2 right     = dir.GetRotatedByMinus90Degrees();
	Vector2 halfShaft = right * (lineThickness * 0.5f);
	float   headWidth = headLength;
	Vector2 halfHead  = right * (headWidth * 0.5f);

	if (shaftLength > 0.0f)
	{
		Vector2 shaftStartL = tailPos + halfShaft;
		Vector2 shaftStartR = tailPos - halfShaft;
		Vector2 shaftEndL   = headBase + halfShaft;
		Vector2 shaftEndR   = headBase - halfShaft;

		verts.emplace_back(Vector3(shaftStartL.x, shaftStartL.y, 0.f), color);
		verts.emplace_back(Vector3(shaftEndL.x, shaftEndL.y, 0.f), color);
		verts.emplace_back(Vector3(shaftEndR.x, shaftEndR.y, 0.f), color);

		verts.emplace_back(Vector3(shaftStartL.x, shaftStartL.y, 0.f), color);
		verts.emplace_back(Vector3(shaftEndR.x, shaftEndR.y, 0.f), color);
		verts.emplace_back(Vector3(shaftStartR.x, shaftStartR.y, 0.f), color);
	}

	Vector2 headRight = headBase + halfHead;
	Vector2 headLeft  = headBase - halfHead;

	verts.emplace_back(Vector3(tipPos.x, tipPos.y, 0.f), color);
	verts.emplace_back(Vector3(headLeft.x, headLeft.y, 0.f), color);
	verts.emplace_back(Vector3(headRight.x, headRight.y, 0.f), color);
}

void AddVertsForQuad3D(
	std::vector<Vertex>& verts,
	const Vector3&       bottomLeft,
	const Vector3&       bottomRight,
	const Vector3&       topRight,
	const Vector3&       topLeft,
	const Color&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit */
)
{
	Vector2 uvMins = UVs.m_mins;
	Vector2 uvMaxs = UVs.m_maxs;

	Vector3 const tangent   = (bottomRight - bottomLeft).GetNormalized();
	Vector3 const bitangent = (topLeft - bottomLeft).GetNormalized();
	Vector3 const normal    = CrossProduct3D(tangent, bitangent).GetNormalized();

	verts.emplace_back(bottomLeft, color, Vector2(uvMins.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(bottomRight, color, Vector2(uvMaxs.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(topRight, color, Vector2(uvMaxs.x, uvMaxs.y), tangent, bitangent, normal);

	verts.emplace_back(bottomLeft, color, Vector2(uvMins.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(topRight, color, Vector2(uvMaxs.x, uvMaxs.y), tangent, bitangent, normal);
	verts.emplace_back(topLeft, color, Vector2(uvMins.x, uvMaxs.y), tangent, bitangent, normal);
}

void AddVertsForQuad3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vector3&             bottomLeft,
	const Vector3&             bottomRight,
	const Vector3&             topRight,
	const Vector3&             topLeft,
	const Color&               color /*= Rgba8::kWhite*/,
	const AABB2&               UVs /*= AABB2::Unit */
)
{
	Vector2 const uvMins = UVs.m_mins;
	Vector2 const uvMaxs = UVs.m_maxs;

	Vector3 const tangent   = (bottomRight - bottomLeft).GetNormalized();
	Vector3 const bitangent = (topLeft - bottomLeft).GetNormalized();
	Vector3 const normal    = CrossProduct3D(tangent, bitangent).GetNormalized();

	unsigned int startIndex = (unsigned int)verts.size();

	verts.emplace_back(bottomLeft, color, Vector2(uvMins.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(bottomRight, color, Vector2(uvMaxs.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(topRight, color, Vector2(uvMaxs.x, uvMaxs.y), tangent, bitangent, normal);
	verts.emplace_back(topLeft, color, Vector2(uvMins.x, uvMaxs.y), tangent, bitangent, normal);

	indexes.push_back(startIndex + 0);
	indexes.push_back(startIndex + 1);
	indexes.push_back(startIndex + 2);

	indexes.push_back(startIndex + 0);
	indexes.push_back(startIndex + 2);
	indexes.push_back(startIndex + 3);
}

void AddVertsForAABB3D(
	std::vector<Vertex>& verts,
	const AABB3&         bounds,
	const Color&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/
)
{
	Vector3 const& mins = bounds.m_mins;
	Vector3 const& maxs = bounds.m_maxs;

	// +Z (top)
	AddVertsForQuad3D(
		verts,
		Vector3(mins.x, mins.y, maxs.z),
		Vector3(maxs.x, mins.y, maxs.z),
		Vector3(maxs.x, maxs.y, maxs.z),
		Vector3(mins.x, maxs.y, maxs.z),
		color,
		UVs);

	// -Z (bottom)
	AddVertsForQuad3D(
		verts,
		Vector3(mins.x, maxs.y, mins.z),
		Vector3(maxs.x, maxs.y, mins.z),
		Vector3(maxs.x, mins.y, mins.z),
		Vector3(mins.x, mins.y, mins.z),
		color,
		UVs);

	// +X
	AddVertsForQuad3D(
		verts,
		Vector3(maxs.x, mins.y, mins.z),
		Vector3(maxs.x, maxs.y, mins.z),
		Vector3(maxs.x, maxs.y, maxs.z),
		Vector3(maxs.x, mins.y, maxs.z),
		color,
		UVs);

	// -X
	AddVertsForQuad3D(
		verts,
		Vector3(mins.x, maxs.y, mins.z),
		Vector3(mins.x, mins.y, mins.z),
		Vector3(mins.x, mins.y, maxs.z),
		Vector3(mins.x, maxs.y, maxs.z),
		color,
		UVs);

	// +Y
	AddVertsForQuad3D(
		verts,
		Vector3(maxs.x, maxs.y, mins.z),
		Vector3(mins.x, maxs.y, mins.z),
		Vector3(mins.x, maxs.y, maxs.z),
		Vector3(maxs.x, maxs.y, maxs.z),
		color,
		UVs);

	// -Y
	AddVertsForQuad3D(
		verts,
		Vector3(mins.x, mins.y, mins.z),
		Vector3(maxs.x, mins.y, mins.z),
		Vector3(maxs.x, mins.y, maxs.z),
		Vector3(mins.x, mins.y, maxs.z),
		color,
		UVs);
}

void AddVertsForAABB3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const AABB3&               bounds,
	const Color&               color,
	const AABB2&               UVs)
{
	Vector3 const& mins = bounds.m_mins;
	Vector3 const& maxs = bounds.m_maxs;

	// +Z (top)
	AddVertsForQuad3D(
		verts,
		indexes,
		Vector3(mins.x, mins.y, maxs.z),
		Vector3(maxs.x, mins.y, maxs.z),
		Vector3(maxs.x, maxs.y, maxs.z),
		Vector3(mins.x, maxs.y, maxs.z),
		color,
		UVs);

	// -Z (bottom)
	AddVertsForQuad3D(
		verts,
		indexes,
		Vector3(mins.x, maxs.y, mins.z),
		Vector3(maxs.x, maxs.y, mins.z),
		Vector3(maxs.x, mins.y, mins.z),
		Vector3(mins.x, mins.y, mins.z),
		color,
		UVs);

	// +X
	AddVertsForQuad3D(
		verts,
		indexes,
		Vector3(maxs.x, mins.y, mins.z),
		Vector3(maxs.x, maxs.y, mins.z),
		Vector3(maxs.x, maxs.y, maxs.z),
		Vector3(maxs.x, mins.y, maxs.z),
		color,
		UVs);

	// -X
	AddVertsForQuad3D(
		verts,
		indexes,
		Vector3(mins.x, maxs.y, mins.z),
		Vector3(mins.x, mins.y, mins.z),
		Vector3(mins.x, mins.y, maxs.z),
		Vector3(mins.x, maxs.y, maxs.z),
		color,
		UVs);

	// +Y
	AddVertsForQuad3D(
		verts,
		indexes,
		Vector3(maxs.x, maxs.y, mins.z),
		Vector3(mins.x, maxs.y, mins.z),
		Vector3(mins.x, maxs.y, maxs.z),
		Vector3(maxs.x, maxs.y, maxs.z),
		color,
		UVs);

	// -Y
	AddVertsForQuad3D(
		verts,
		indexes,
		Vector3(mins.x, mins.y, mins.z),
		Vector3(maxs.x, mins.y, mins.z),
		Vector3(maxs.x, mins.y, maxs.z),
		Vector3(mins.x, mins.y, maxs.z),
		color,
		UVs);
}

void AddVertsForSphere3D(
	std::vector<Vertex>& verts,
	const Vector3&       center,
	float                radius,
	const Color&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);
	numStacks = Max(2, numStacks);

	float uRange = UVs.m_maxs.x - UVs.m_mins.x;
	float vRange = UVs.m_maxs.y - UVs.m_mins.y;

	auto AddSphereVertex = [&](Vector3 const& position, float u, float v, float yaw)
	{
		Vector3 const normal    = (position - center).GetNormalized();
		Vector3 const tangent   = Vector3(-sinf(yaw), cosf(yaw), 0.f).GetNormalized();
		Vector3 const bitangent = CrossProduct3D(normal, tangent).GetNormalized();
		verts.emplace_back(position, color, Vector2(u, v), tangent, bitangent, normal);
	};

	for (int stackIndex = 0; stackIndex < numStacks; ++stackIndex)
	{
		float v0Frac = static_cast<float>(stackIndex) / static_cast<float>(numStacks);
		float v1Frac = static_cast<float>(stackIndex + 1) / static_cast<float>(numStacks);

		float pitch0 = -kHalfPi + v0Frac * kPi;
		float pitch1 = -kHalfPi + v1Frac * kPi;

		float v0 = UVs.m_mins.y + v0Frac * vRange;
		float v1 = UVs.m_mins.y + v1Frac * vRange;

		for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
		{
			float u0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
			float u1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);

			float yaw0 = u0Frac * kTwoPi;
			float yaw1 = u1Frac * kTwoPi;

			Vector3 p00 = center + Vector3::MakeFromPolarRadians(pitch0, yaw0, radius);
			Vector3 p10 = center + Vector3::MakeFromPolarRadians(pitch0, yaw1, radius);
			Vector3 p11 = center + Vector3::MakeFromPolarRadians(pitch1, yaw1, radius);
			Vector3 p01 = center + Vector3::MakeFromPolarRadians(pitch1, yaw0, radius);

			float u0 = UVs.m_mins.x + u0Frac * uRange;
			float u1 = UVs.m_mins.x + u1Frac * uRange;

			AddSphereVertex(p00, u0, v0, yaw0);
			AddSphereVertex(p10, u1, v0, yaw1);
			AddSphereVertex(p11, u1, v1, yaw1);

			AddSphereVertex(p00, u0, v0, yaw0);
			AddSphereVertex(p11, u1, v1, yaw1);
			AddSphereVertex(p01, u0, v1, yaw0);
		}
	}
}

void AddVertsForSphere3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vector3&             center,
	float                      radius,
	const Color&               color /*= Rgba8::kWhite*/,
	const AABB2&               UVs /*= AABB2::Unit*/,
	int                        numSlices /*= 16*/,
	int                        numStacks /*= 8*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);
	numStacks = Max(2, numStacks);

	float uRange = UVs.m_maxs.x - UVs.m_mins.x;
	float vRange = UVs.m_maxs.y - UVs.m_mins.y;

	unsigned int const baseIndex = static_cast<unsigned int>(verts.size());

	// Unique vertices: (numStacks+1) rows × numSlices columns
	for (int stackIndex = 0; stackIndex <= numStacks; ++stackIndex)
	{
		float vFrac = static_cast<float>(stackIndex) / static_cast<float>(numStacks);
		float pitch = -kHalfPi + vFrac * kPi;
		float v     = UVs.m_mins.y + vFrac * vRange;

		for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
		{
			float uFrac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
			float yaw   = uFrac * kTwoPi;
			float u     = UVs.m_mins.x + uFrac * uRange;

			Vector3 pos       = center + Vector3::MakeFromPolarRadians(pitch, yaw, radius);
			Vector3 normal    = (pos - center).GetNormalized();
			Vector3 tangent   = Vector3(-sinf(yaw), cosf(yaw), 0.f).GetNormalized();
			Vector3 bitangent = CrossProduct3D(normal, tangent).GetNormalized();

			verts.emplace_back(pos, color, Vector2(u, v), tangent, bitangent, normal);
		}
	}

	// Indices: for each quad cell, 2 triangles using vertex sharing
	for (int stackIndex = 0; stackIndex < numStacks; ++stackIndex)
	{
		for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
		{
			unsigned int const nextSlice = (sliceIndex + 1) % numSlices;

			unsigned int const v00 = baseIndex + stackIndex * numSlices + sliceIndex;
			unsigned int const v10 = baseIndex + stackIndex * numSlices + nextSlice;
			unsigned int const v11 = baseIndex + (stackIndex + 1) * numSlices + nextSlice;
			unsigned int const v01 = baseIndex + (stackIndex + 1) * numSlices + sliceIndex;

			indexes.push_back(v00);
			indexes.push_back(v10);
			indexes.push_back(v11);
			indexes.push_back(v00);
			indexes.push_back(v11);
			indexes.push_back(v01);
		}
	}
}

void AddVertsForCylinder3D(
	std::vector<Vertex>& verts,
	const Vector3&       start,
	const Vector3&       end,
	float                radius,
	const Color&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/,
	int                  numSlices /*= 16*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vector3 axis   = end - start;
	float   height = axis.GetLength();
	if (height <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);

	Vector3 kBasis = axis / height;

	// Pick a helper not parallel to kBasis
	Vector3 helper = (Abs(kBasis.z) < 0.999f) ? Vector3(0.f, 0.f, 1.f) : Vector3(0.f, 1.f, 0.f);

	Vector3 iBasis = CrossProduct3D(helper, kBasis);
	float   iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;

	Vector3 jBasis = CrossProduct3D(kBasis, iBasis); // already normalized if i,k are orthonormal

	float uRange = UVs.m_maxs.x - UVs.m_mins.x;
	float vRange = UVs.m_maxs.y - UVs.m_mins.y;

	float deltaYaw = kTwoPi / static_cast<float>(numSlices);

	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float yaw0 = deltaYaw * static_cast<float>(sliceIndex);
		float yaw1 = deltaYaw * static_cast<float>(sliceIndex + 1);

		float cos0 = cosf(yaw0);
		float sin0 = sinf(yaw0);
		float cos1 = cosf(yaw1);
		float sin1 = sinf(yaw1);

		Vector3 rim0 = (iBasis * cos0 + jBasis * sin0) * radius;
		Vector3 rim1 = (iBasis * cos1 + jBasis * sin1) * radius;

		Vector3 b0 = start + rim0;
		Vector3 b1 = start + rim1;
		Vector3 t0 = end + rim0;
		Vector3 t1 = end + rim1;

		Vector3 const radial0       = rim0.GetNormalized();
		Vector3 const radial1       = rim1.GetNormalized();
		Vector3 const tangent0      = (-iBasis * sin0 + jBasis * cos0).GetNormalized();
		Vector3 const tangent1      = (-iBasis * sin1 + jBasis * cos1).GetNormalized();
		Vector3 const sideBitangent = kBasis;

		float u0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);

		float u0 = UVs.m_mins.x + u0Frac * uRange;
		float u1 = UVs.m_mins.x + u1Frac * uRange;

		float vBottom = UVs.m_mins.y;
		float vTop    = UVs.m_mins.y + vRange;

		// Side quad (two triangles)
		verts.emplace_back(b0, color, Vector2(u0, vBottom), tangent0, sideBitangent, radial0);
		verts.emplace_back(b1, color, Vector2(u1, vBottom), tangent1, sideBitangent, radial1);
		verts.emplace_back(t1, color, Vector2(u1, vTop), tangent1, sideBitangent, radial1);

		verts.emplace_back(b0, color, Vector2(u0, vBottom), tangent0, sideBitangent, radial0);
		verts.emplace_back(t1, color, Vector2(u1, vTop), tangent1, sideBitangent, radial1);
		verts.emplace_back(t0, color, Vector2(u0, vTop), tangent0, sideBitangent, radial0);

		// Bottom cap (-kBasis) - triangle fan
		Vector3 cB  = start;
		float   cu0 = UVs.m_mins.x + (0.5f + 0.5f * cos0) * uRange;
		float   cv0 = UVs.m_mins.y + (0.5f + 0.5f * sin0) * vRange;
		float   cu1 = UVs.m_mins.x + (0.5f + 0.5f * cos1) * uRange;
		float   cv1 = UVs.m_mins.y + (0.5f + 0.5f * sin1) * vRange;
		float   cuC = UVs.m_mins.x + 0.5f * uRange;
		float   cvC = UVs.m_mins.y + 0.5f * vRange;

		// Winding chosen to face outward on bottom
		Vector3 const bottomNormal    = -kBasis;
		Vector3 const bottomTangent   = iBasis;
		Vector3 const bottomBitangent = -jBasis;
		verts.emplace_back(cB, color, Vector2(cuC, cvC), bottomTangent, bottomBitangent, bottomNormal);
		verts.emplace_back(b1, color, Vector2(cu1, cv1), bottomTangent, bottomBitangent, bottomNormal);
		verts.emplace_back(b0, color, Vector2(cu0, cv0), bottomTangent, bottomBitangent, bottomNormal);

		// Top cap (+kBasis)
		Vector3 cT = end;
		// Winding chosen to face outward on top
		verts.emplace_back(cT, color, Vector2(cuC, cvC), iBasis, jBasis, kBasis);
		verts.emplace_back(t0, color, Vector2(cu0, cv0), iBasis, jBasis, kBasis);
		verts.emplace_back(t1, color, Vector2(cu1, cv1), iBasis, jBasis, kBasis);
	}
}

void AddVertsForCylinder3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vector3&             start,
	const Vector3&             end,
	float                      radius,
	const Color&               color /*= Rgba8::kWhite*/,
	const AABB2&               UVs /*= AABB2::Unit*/,
	int                        numSlices /*= 16*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vector3 axis   = end - start;
	float   height = axis.GetLength();
	if (height <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);

	Vector3 kBasis = axis / height;
	Vector3 helper = (Abs(kBasis.z) < 0.999f) ? Vector3(0.f, 0.f, 1.f) : Vector3(0.f, 1.f, 0.f);
	Vector3 iBasis = CrossProduct3D(helper, kBasis);
	float   iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;
	Vector3 jBasis = CrossProduct3D(kBasis, iBasis);

	float uRange   = UVs.m_maxs.x - UVs.m_mins.x;
	float vRange   = UVs.m_maxs.y - UVs.m_mins.y;
	float deltaYaw = kTwoPi / static_cast<float>(numSlices);

	// Side rim vertices: bottom + top, radial normals (used by side quads only)
	unsigned int const botRimSideBase = static_cast<unsigned int>(verts.size());
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float   yaw     = deltaYaw * static_cast<float>(sliceIndex);
		float   c       = cosf(yaw);
		float   s       = sinf(yaw);
		Vector3 rim     = (iBasis * c + jBasis * s) * radius;
		Vector3 radial  = rim.GetNormalized();
		Vector3 tangent = (-iBasis * s + jBasis * c).GetNormalized();

		float uFrac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u     = UVs.m_mins.x + uFrac * uRange;

		verts.emplace_back(start + rim, color, Vector2(u, UVs.m_mins.y), tangent, kBasis, radial);
	}

	unsigned int const topRimSideBase = static_cast<unsigned int>(verts.size());
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float   yaw     = deltaYaw * static_cast<float>(sliceIndex);
		float   c       = cosf(yaw);
		float   s       = sinf(yaw);
		Vector3 rim     = (iBasis * c + jBasis * s) * radius;
		Vector3 radial  = rim.GetNormalized();
		Vector3 tangent = (-iBasis * s + jBasis * c).GetNormalized();

		float uFrac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u     = UVs.m_mins.x + uFrac * uRange;

		verts.emplace_back(end + rim, color, Vector2(u, UVs.m_mins.y + vRange), tangent, kBasis, radial);
	}

	// Cap rim vertices: bottom + top, axis-aligned normals (used by caps only)
	float const cuC = UVs.m_mins.x + 0.5f * uRange;
	float const cvC = UVs.m_mins.y + 0.5f * vRange;

	unsigned int const botRimCapBase   = static_cast<unsigned int>(verts.size());
	Vector3 const      bottomNormal    = -kBasis;
	Vector3 const      bottomTangent   = iBasis;
	Vector3 const      bottomBitangent = -jBasis;
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float   yaw = deltaYaw * static_cast<float>(sliceIndex);
		float   c   = cosf(yaw);
		float   s   = sinf(yaw);
		Vector3 rim = (iBasis * c + jBasis * s) * radius;

		float cu = UVs.m_mins.x + (0.5f + 0.5f * c) * uRange;
		float cv = UVs.m_mins.y + (0.5f + 0.5f * s) * vRange;
		verts.emplace_back(start + rim, color, Vector2(cu, cv), bottomTangent, bottomBitangent, bottomNormal);
	}

	unsigned int const topRimCapBase = static_cast<unsigned int>(verts.size());
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float   yaw = deltaYaw * static_cast<float>(sliceIndex);
		float   c   = cosf(yaw);
		float   s   = sinf(yaw);
		Vector3 rim = (iBasis * c + jBasis * s) * radius;

		float cu = UVs.m_mins.x + (0.5f + 0.5f * c) * uRange;
		float cv = UVs.m_mins.y + (0.5f + 0.5f * s) * vRange;
		verts.emplace_back(end + rim, color, Vector2(cu, cv), iBasis, jBasis, kBasis);
	}

	// Center vertices
	unsigned int const botCenterIdx = static_cast<unsigned int>(verts.size());
	verts.emplace_back(start, color, Vector2(cuC, cvC), bottomTangent, bottomBitangent, bottomNormal);

	unsigned int const topCenterIdx = static_cast<unsigned int>(verts.size());
	verts.emplace_back(end, color, Vector2(cuC, cvC), iBasis, jBasis, kBasis);

	// Side indices: reference side rim vertices only
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		unsigned int const next = (sliceIndex + 1) % numSlices;
		unsigned int const b0   = botRimSideBase + sliceIndex;
		unsigned int const b1   = botRimSideBase + next;
		unsigned int const t0   = topRimSideBase + sliceIndex;
		unsigned int const t1   = topRimSideBase + next;

		indexes.push_back(b0);
		indexes.push_back(b1);
		indexes.push_back(t1);
		indexes.push_back(b0);
		indexes.push_back(t1);
		indexes.push_back(t0);
	}

	// Bottom cap: reference cap rim vertices only
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		unsigned int const next = (sliceIndex + 1) % numSlices;
		indexes.push_back(botCenterIdx);
		indexes.push_back(botRimCapBase + next);
		indexes.push_back(botRimCapBase + sliceIndex);
	}

	// Top cap: reference cap rim vertices only
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		unsigned int const next = (sliceIndex + 1) % numSlices;
		indexes.push_back(topCenterIdx);
		indexes.push_back(topRimCapBase + sliceIndex);
		indexes.push_back(topRimCapBase + next);
	}
}

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Vector3 const&       start,
	Vector3 const&       end,
	float                radius,
	AABB2 const&         UVs,
	Color const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vector3 const axis       = end - start;
	float const   axisLength = axis.GetLength();
	if (axisLength <= 0.f)
	{
		AddVertsForSphere3D(verts, start, radius, color, UVs, numSlices, numStacks);
		return;
	}

	numSlices = Max(3, numSlices);
	numStacks = Max(2, numStacks);

	Vector3 kBasis = axis / axisLength;

	Vector3 helper = (Abs(kBasis.z) < 0.999f) ? Vector3(0.f, 0.f, 1.f) : Vector3(0.f, 1.f, 0.f);
	Vector3 iBasis = CrossProduct3D(helper, kBasis);
	float   iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;
	Vector3 jBasis = CrossProduct3D(kBasis, iBasis);

	float const deltaYaw   = kTwoPi / static_cast<float>(numSlices);
	int const   hemiStacks = Max(1, numStacks / 2);

	float const uRange       = UVs.m_maxs.x - UVs.m_mins.x;
	float const vRange       = UVs.m_maxs.y - UVs.m_mins.y;
	float const totalVLength = axisLength + 2.f * radius;
	float const bodyVMinFrac = radius / totalVLength;
	float const bodyVMaxFrac = (radius + axisLength) / totalVLength;

	auto GetRadialDir = [&](float yaw) -> Vector3 { return iBasis * cosf(yaw) + jBasis * sinf(yaw); };

	auto GetHemispherePoint = [&](Vector3 const& center, float yaw, float pitch) -> Vector3
	{
		float radialScale = cosf(pitch);
		return center + GetRadialDir(yaw) * (radius * radialScale) + kBasis * (radius * sinf(pitch));
	};

	auto GetU = [&](float yawFrac) -> float { return UVs.m_mins.x + yawFrac * uRange; };

	auto GetV = [&](float vFrac) -> float { return UVs.m_mins.y + vFrac * vRange; };

	auto AddCapsuleVertex = [&](Vector3 const& position, Vector3 const& normal, float yaw, float u, float v)
	{
		Vector3 const tangent   = (-iBasis * sinf(yaw) + jBasis * cosf(yaw)).GetNormalized();
		Vector3 const bitangent = CrossProduct3D(normal, tangent).GetNormalized();
		verts.emplace_back(position, color, Vector2(u, v), tangent, bitangent, normal);
	};

	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float const yaw0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float const yaw1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);
		float const yaw0     = deltaYaw * static_cast<float>(sliceIndex);
		float const yaw1     = deltaYaw * static_cast<float>(sliceIndex + 1);

		Vector3 const rim0 = GetRadialDir(yaw0) * radius;
		Vector3 const rim1 = GetRadialDir(yaw1) * radius;

		Vector3 const b0      = start + rim0;
		Vector3 const b1      = start + rim1;
		Vector3 const t0      = end + rim0;
		Vector3 const t1      = end + rim1;
		Vector3 const normal0 = rim0.GetNormalized();
		Vector3 const normal1 = rim1.GetNormalized();

		float const u0      = GetU(yaw0Frac);
		float const u1      = GetU(yaw1Frac);
		float const vBottom = GetV(bodyVMinFrac);
		float const vTop    = GetV(bodyVMaxFrac);

		AddCapsuleVertex(b0, normal0, yaw0, u0, vBottom);
		AddCapsuleVertex(b1, normal1, yaw1, u1, vBottom);
		AddCapsuleVertex(t1, normal1, yaw1, u1, vTop);

		AddCapsuleVertex(b0, normal0, yaw0, u0, vBottom);
		AddCapsuleVertex(t1, normal1, yaw1, u1, vTop);
		AddCapsuleVertex(t0, normal0, yaw0, u0, vTop);
	}

	for (int stackIndex = 0; stackIndex < hemiStacks; ++stackIndex)
	{
		float const lowerPitch0 =
			-kHalfPi + (kHalfPi * static_cast<float>(stackIndex) / static_cast<float>(hemiStacks));
		float const lowerPitch1 =
			-kHalfPi + (kHalfPi * static_cast<float>(stackIndex + 1) / static_cast<float>(hemiStacks));
		float const upperPitch0 = kHalfPi * static_cast<float>(stackIndex) / static_cast<float>(hemiStacks);
		float const upperPitch1 = kHalfPi * static_cast<float>(stackIndex + 1) / static_cast<float>(hemiStacks);

		float const lowerV0Frac = (radius * (sinf(lowerPitch0) + 1.f) * 0.5f) / totalVLength;
		float const lowerV1Frac = (radius * (sinf(lowerPitch1) + 1.f) * 0.5f) / totalVLength;
		float const upperV0Frac = (radius + axisLength + radius * sinf(upperPitch0)) / totalVLength;
		float const upperV1Frac = (radius + axisLength + radius * sinf(upperPitch1)) / totalVLength;

		for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
		{
			float const yaw0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
			float const yaw1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);
			float const yaw0     = deltaYaw * static_cast<float>(sliceIndex);
			float const yaw1     = deltaYaw * static_cast<float>(sliceIndex + 1);

			float const u0 = GetU(yaw0Frac);
			float const u1 = GetU(yaw1Frac);

			Vector3 const lower00 = GetHemispherePoint(start, yaw0, lowerPitch0);
			Vector3 const lower10 = GetHemispherePoint(start, yaw1, lowerPitch0);
			Vector3 const lower11 = GetHemispherePoint(start, yaw1, lowerPitch1);
			Vector3 const lower01 = GetHemispherePoint(start, yaw0, lowerPitch1);

			AddCapsuleVertex(lower00, (lower00 - start).GetNormalized(), yaw0, u0, GetV(lowerV0Frac));
			AddCapsuleVertex(lower10, (lower10 - start).GetNormalized(), yaw1, u1, GetV(lowerV0Frac));
			AddCapsuleVertex(lower11, (lower11 - start).GetNormalized(), yaw1, u1, GetV(lowerV1Frac));

			AddCapsuleVertex(lower00, (lower00 - start).GetNormalized(), yaw0, u0, GetV(lowerV0Frac));
			AddCapsuleVertex(lower11, (lower11 - start).GetNormalized(), yaw1, u1, GetV(lowerV1Frac));
			AddCapsuleVertex(lower01, (lower01 - start).GetNormalized(), yaw0, u0, GetV(lowerV1Frac));

			Vector3 const upper00 = GetHemispherePoint(end, yaw0, upperPitch0);
			Vector3 const upper10 = GetHemispherePoint(end, yaw1, upperPitch0);
			Vector3 const upper11 = GetHemispherePoint(end, yaw1, upperPitch1);
			Vector3 const upper01 = GetHemispherePoint(end, yaw0, upperPitch1);

			AddCapsuleVertex(upper00, (upper00 - end).GetNormalized(), yaw0, u0, GetV(upperV0Frac));
			AddCapsuleVertex(upper10, (upper10 - end).GetNormalized(), yaw1, u1, GetV(upperV0Frac));
			AddCapsuleVertex(upper11, (upper11 - end).GetNormalized(), yaw1, u1, GetV(upperV1Frac));

			AddCapsuleVertex(upper00, (upper00 - end).GetNormalized(), yaw0, u0, GetV(upperV0Frac));
			AddCapsuleVertex(upper11, (upper11 - end).GetNormalized(), yaw1, u1, GetV(upperV1Frac));
			AddCapsuleVertex(upper01, (upper01 - end).GetNormalized(), yaw0, u0, GetV(upperV1Frac));
		}
	}
}

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Vector3 const&       start,
	Vector3 const&       end,
	float                radius,
	Color const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	AddVertsForCapsule3D(verts, start, end, radius, AABB2::Unit, color, numSlices, numStacks);
}

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Capsule3 const&      capsule,
	AABB2 const&         UVs,
	Color const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	AddVertsForCapsule3D(verts, capsule.m_start, capsule.m_end, capsule.m_radius, UVs, color, numSlices, numStacks);
}

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Capsule3 const&      capsule,
	Color const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	AddVertsForCapsule3D(
		verts,
		capsule.m_start,
		capsule.m_end,
		capsule.m_radius,
		AABB2::Unit,
		color,
		numSlices,
		numStacks);
}

void AddVertsForCone3D(
	std::vector<Vertex>& verts,
	const Vector3&       start,
	const Vector3&       end,
	float                radius,
	const Color&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/,
	int                  numSlices /*= 32*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vector3 axis   = end - start;
	float   height = axis.GetLength();
	if (height <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);

	Vector3 kBasis = axis / height;

	Vector3 helper = (Abs(kBasis.z) < 0.999f) ? Vector3(0.f, 0.f, 1.f) : Vector3(0.f, 1.f, 0.f);

	Vector3 iBasis = CrossProduct3D(helper, kBasis);
	float   iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;

	Vector3 jBasis = CrossProduct3D(kBasis, iBasis);

	float uRange = UVs.m_maxs.x - UVs.m_mins.x;
	float vRange = UVs.m_maxs.y - UVs.m_mins.y;

	float deltaYaw = kTwoPi / static_cast<float>(numSlices);

	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float yaw0 = deltaYaw * static_cast<float>(sliceIndex);
		float yaw1 = deltaYaw * static_cast<float>(sliceIndex + 1);

		float cos0 = cosf(yaw0);
		float sin0 = sinf(yaw0);
		float cos1 = cosf(yaw1);
		float sin1 = sinf(yaw1);

		Vector3 rim0 = (iBasis * cos0 + jBasis * sin0) * radius;
		Vector3 rim1 = (iBasis * cos1 + jBasis * sin1) * radius;

		Vector3 b0           = start + rim0;
		Vector3 b1           = start + rim1;
		Vector3 tip          = end;
		Vector3 tangent0     = (-iBasis * sin0 + jBasis * cos0).GetNormalized();
		Vector3 tangent1     = (-iBasis * sin1 + jBasis * cos1).GetNormalized();
		Vector3 bitangent0   = (kBasis * height - rim0).GetNormalized();
		Vector3 bitangent1   = (kBasis * height - rim1).GetNormalized();
		Vector3 normal0      = CrossProduct3D(tangent0, bitangent0).GetNormalized();
		Vector3 normal1      = CrossProduct3D(tangent1, bitangent1).GetNormalized();
		Vector3 tipTangent   = (tangent0 + tangent1).GetNormalized();
		Vector3 tipBitangent = (bitangent0 + bitangent1).GetNormalized();
		Vector3 tipNormal    = CrossProduct3D(tipTangent, tipBitangent).GetNormalized();

		float u0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);

		float u0 = UVs.m_mins.x + u0Frac * uRange;
		float u1 = UVs.m_mins.x + u1Frac * uRange;

		float vBase = UVs.m_mins.y;
		float vTip  = UVs.m_mins.y + vRange;

		// Side triangle
		verts.emplace_back(b0, color, Vector2(u0, vBase), tangent0, bitangent0, normal0);
		verts.emplace_back(b1, color, Vector2(u1, vBase), tangent1, bitangent1, normal1);
		verts.emplace_back(tip, color, Vector2((u0 + u1) * 0.5f, vTip), tipTangent, tipBitangent, tipNormal);

		// Base cap (faces outward opposite to +kBasis => -kBasis)
		Vector3 cB = start;

		float cu0 = UVs.m_mins.x + (0.5f + 0.5f * cos0) * uRange;
		float cv0 = UVs.m_mins.y + (0.5f + 0.5f * sin0) * vRange;
		float cu1 = UVs.m_mins.x + (0.5f + 0.5f * cos1) * uRange;
		float cv1 = UVs.m_mins.y + (0.5f + 0.5f * sin1) * vRange;
		float cuC = UVs.m_mins.x + 0.5f * uRange;
		float cvC = UVs.m_mins.y + 0.5f * vRange;

		Vector3 const baseNormal    = -kBasis;
		Vector3 const baseTangent   = iBasis;
		Vector3 const baseBitangent = -jBasis;
		verts.emplace_back(cB, color, Vector2(cuC, cvC), baseTangent, baseBitangent, baseNormal);
		verts.emplace_back(b1, color, Vector2(cu1, cv1), baseTangent, baseBitangent, baseNormal);
		verts.emplace_back(b0, color, Vector2(cu0, cv0), baseTangent, baseBitangent, baseNormal);
	}
}

void AddVertsForCone3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vector3&             start,
	const Vector3&             end,
	float                      radius,
	const Color&               color /*= Rgba8::kWhite*/,
	const AABB2&               UVs /*= AABB2::Unit*/,
	int                        numSlices /*= 32*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vector3 axis   = end - start;
	float   height = axis.GetLength();
	if (height <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);

	Vector3 kBasis = axis / height;
	Vector3 helper = (Abs(kBasis.z) < 0.999f) ? Vector3(0.f, 0.f, 1.f) : Vector3(0.f, 1.f, 0.f);
	Vector3 iBasis = CrossProduct3D(helper, kBasis);
	float   iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;
	Vector3 jBasis = CrossProduct3D(kBasis, iBasis);

	float uRange   = UVs.m_maxs.x - UVs.m_mins.x;
	float vRange   = UVs.m_maxs.y - UVs.m_mins.y;
	float deltaYaw = kTwoPi / static_cast<float>(numSlices);

	// Side rim vertices: cone surface normals (used by side triangles only)
	unsigned int const botRimSideBase = static_cast<unsigned int>(verts.size());
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float   yaw       = deltaYaw * static_cast<float>(sliceIndex);
		float   c         = cosf(yaw);
		float   s         = sinf(yaw);
		Vector3 rim       = (iBasis * c + jBasis * s) * radius;
		Vector3 pos       = start + rim;
		Vector3 tangent   = (-iBasis * s + jBasis * c).GetNormalized();
		Vector3 bitangent = (kBasis * height - rim).GetNormalized();
		Vector3 normal    = CrossProduct3D(tangent, bitangent).GetNormalized();

		float uFrac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u     = UVs.m_mins.x + uFrac * uRange;

		verts.emplace_back(pos, color, Vector2(u, UVs.m_mins.y), tangent, bitangent, normal);
	}

	// Tip vertex
	unsigned int const tipIdx       = static_cast<unsigned int>(verts.size());
	Vector3 const      tipTangent   = iBasis;
	Vector3 const      tipBitangent = jBasis;
	Vector3 const      tipNormal    = kBasis;
	verts.emplace_back(
		end,
		color,
		Vector2(UVs.m_mins.x + 0.5f * uRange, UVs.m_mins.y + vRange),
		tipTangent,
		tipBitangent,
		tipNormal);

	// Cap rim vertices: axis-aligned normals (used by bottom cap only)
	unsigned int const botRimCapBase = static_cast<unsigned int>(verts.size());
	Vector3 const      baseNormal    = -kBasis;
	Vector3 const      baseTangent   = iBasis;
	Vector3 const      baseBitangent = -jBasis;
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float   yaw = deltaYaw * static_cast<float>(sliceIndex);
		float   c   = cosf(yaw);
		float   s   = sinf(yaw);
		Vector3 rim = (iBasis * c + jBasis * s) * radius;

		float cu = UVs.m_mins.x + (0.5f + 0.5f * c) * uRange;
		float cv = UVs.m_mins.y + (0.5f + 0.5f * s) * vRange;
		verts.emplace_back(start + rim, color, Vector2(cu, cv), baseTangent, baseBitangent, baseNormal);
	}

	// Bottom center
	unsigned int const botCenterIdx = static_cast<unsigned int>(verts.size());
	float const        cuC          = UVs.m_mins.x + 0.5f * uRange;
	float const        cvC          = UVs.m_mins.y + 0.5f * vRange;
	verts.emplace_back(start, color, Vector2(cuC, cvC), baseTangent, baseBitangent, baseNormal);

	// Side triangles: reference side rim vertices only
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		unsigned int const next = (sliceIndex + 1) % numSlices;
		indexes.push_back(botRimSideBase + sliceIndex);
		indexes.push_back(botRimSideBase + next);
		indexes.push_back(tipIdx);
	}

	// Bottom cap: reference cap rim vertices only
	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		unsigned int const next = (sliceIndex + 1) % numSlices;
		indexes.push_back(botCenterIdx);
		indexes.push_back(botRimCapBase + next);
		indexes.push_back(botRimCapBase + sliceIndex);
	}
}

void AddVertsForArrow3D(
	std::vector<Vertex>& verts,
	Vector3 const&       start,
	Vector3 const&       end,
	float                radius,
	Color const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/
)
{
	Vector3 dir    = end - start;
	float   length = dir.GetLength();
	if (length <= 0.f || radius <= 0.f)
	{
		return;
	}

	// Keep a visible shaft while making the head read clearly at gameplay distances.
	float headLength = Min(length * 0.30f, radius * 6.f);
	headLength       = GetClamped(headLength, length * 0.20f, length * 0.45f);

	float shaftLength = length - headLength;
	if (shaftLength < 0.f)
	{
		shaftLength = 0.f;
		headLength  = length;
	}

	Vector3 shaftEnd    = start + (dir / length) * shaftLength;
	float   shaftRadius = radius * 0.30f;

	AddVertsForCylinder3D(verts, start, shaftEnd, shaftRadius, color, AABB2::Unit, numSlices);
	AddVertsForCone3D(verts, shaftEnd, end, radius, color, AABB2::Unit, numSlices);
}

void AddVertsForDisc2D(std::vector<Vertex>& verts, Disc2 const& disc, Color color)
{
	AddVertsForDisc2D(verts, disc.m_center, disc.m_radius, color);
}

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Capsule2 const& capsule, Color color)
{
	AddVertsForCapsule2D(verts, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius, color);
}

void AddVertsForTriangle2D(std::vector<Vertex>& verts, Triangle2 const& triangle, Color color)
{
	AddVertsForTriangle2D(
		verts,
		triangle.m_pointsCounterClockwise[0],
		triangle.m_pointsCounterClockwise[1],
		triangle.m_pointsCounterClockwise[2],
		color);
}

void AddVertsForLineSegment2D(std::vector<Vertex>& verts, LineSegment2 const& lineSegment, float thickness, Color color)
{
	AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, Vector2(thickness, thickness), color);
}

void AddVertsForInfiniteLine2D(
	std::vector<Vertex>& verts, LineSegment2 const& infiniteLine, float thickness, Color color)
{
	AddVertsForInfiniteLine2D(verts, infiniteLine.m_start, infiniteLine.m_end, thickness, color);
}
