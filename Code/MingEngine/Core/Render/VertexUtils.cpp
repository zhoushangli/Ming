#include "MingEngine/Core/Render/VertexUtils.hpp"

#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"

void TransformVertexArrayXY3D(
	int numVerts, Vertex* verts, float scaleXY, float rotationDegreesAboutZ, Vec2 const& translationXY
)
{
	Vec2 iBasis = Vec2::MakeFromPolarDegrees(rotationDegreesAboutZ, scaleXY);
	Vec2 jBasis = iBasis.GetRotatedBy90Degrees();
	for (int index = 0; index < numVerts; ++index)
	{
		Vec3& position = verts[index].m_position;
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
		bounds.StretchToIncludePoint(Vec2(vert.m_position.x, vert.m_position.y));
	}

	return bounds;
}

AABB3 GetVertexBounds3D(std::vector<Vertex> const& vertices)
{
	if (vertices.empty())
	{
		return AABB3::Zero;
	}

	AABB3 bounds;
	for (Vertex const& vertex : vertices)
	{
		Vec3 const& p = vertex.m_position;
		bounds.StretchToIncludePoint(p);
	}

	return bounds;
}

void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Rgba8 color)
{
	AddVertsForAABB2D(verts, alignedBox, color, Vec2::Zero, Vec2::One);
}

void AddVertsForAABB2D(std::vector<Vertex>& verts, AABB2 const& alignedBox, Rgba8 color, Vec2 uvAtMins, Vec2 uvAtMaxs)
{
	Vec2 mins = alignedBox.m_mins;
	Vec2 maxs = alignedBox.m_maxs;

	verts.emplace_back(Vec3(mins.x, mins.y, 0.f), color, Vec2(uvAtMins.x, uvAtMins.y));
	verts.emplace_back(Vec3(maxs.x, mins.y, 0.f), color, Vec2(uvAtMaxs.x, uvAtMins.y));
	verts.emplace_back(Vec3(maxs.x, maxs.y, 0.f), color, Vec2(uvAtMaxs.x, uvAtMaxs.y));

	verts.emplace_back(Vec3(mins.x, mins.y, 0.f), color, Vec2(uvAtMins.x, uvAtMins.y));
	verts.emplace_back(Vec3(maxs.x, maxs.y, 0.f), color, Vec2(uvAtMaxs.x, uvAtMaxs.y));
	verts.emplace_back(Vec3(mins.x, maxs.y, 0.f), color, Vec2(uvAtMins.x, uvAtMaxs.y));
}

void AddVertsForDisc2D(std::vector<Vertex>& verts, Vec2 discCenter, float discRadius, Rgba8 color)
{
	int numSides = (int)RangeMapClamped(discRadius, 1.f, 10.f, 12.f, 48.f);

	float delta = 360.0f / (float)numSides;
	for (int i = 0; i < numSides; ++i)
	{
		float angle0 = i * delta;
		float angle1 = (i + 1) * delta;
		Vec2  p0     = discCenter;
		Vec2  p1     = discCenter + Vec2::MakeFromPolarDegrees(angle0, discRadius);
		Vec2  p2     = discCenter + Vec2::MakeFromPolarDegrees(angle1, discRadius);
		verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
		verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
		verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);
	}
}

void AddVertsForRing2D(std::vector<Vertex>& verts, Vec2 ringCenter, float ringRadius, float thickness, Rgba8 color)
{
	int numSides = (int)RangeMapClamped(ringRadius, 1.f, 10.f, 36.f, 360.f);

	float innerRadius = ringRadius - thickness * 0.5f;
	float outerRadius = ringRadius + thickness * 0.5f;

	float delta = 360.0f / (float)numSides;
	for (int i = 0; i < numSides; ++i)
	{
		float angle0 = i * delta;
		float angle1 = (i + 1) * delta;
		Vec2  inner0 = ringCenter + Vec2::MakeFromPolarDegrees(angle0, innerRadius);
		Vec2  inner1 = ringCenter + Vec2::MakeFromPolarDegrees(angle1, innerRadius);
		Vec2  outer0 = ringCenter + Vec2::MakeFromPolarDegrees(angle0, outerRadius);
		Vec2  outer1 = ringCenter + Vec2::MakeFromPolarDegrees(angle1, outerRadius);

		// First triangle
		verts.emplace_back(Vec3(inner0.x, inner0.y, 0.f), color);
		verts.emplace_back(Vec3(outer0.x, outer0.y, 0.f), color);
		verts.emplace_back(Vec3(outer1.x, outer1.y, 0.f), color);

		// Second triangle
		verts.emplace_back(Vec3(inner0.x, inner0.y, 0.f), color);
		verts.emplace_back(Vec3(outer1.x, outer1.y, 0.f), color);
		verts.emplace_back(Vec3(inner1.x, inner1.y, 0.f), color);
	}
}

void AddVertsForOBB2D(std::vector<Vertex>& verts, OBB2 const& orientedBox, Rgba8 color)
{
	Vec2 corners[4];
	orientedBox.GetCornerPoints(corners);

	// Two triangles
	verts.emplace_back(Vec3(corners[0].x, corners[0].y, 0.f), color);
	verts.emplace_back(Vec3(corners[1].x, corners[1].y, 0.f), color);
	verts.emplace_back(Vec3(corners[2].x, corners[2].y, 0.f), color);

	verts.emplace_back(Vec3(corners[0].x, corners[0].y, 0.f), color);
	verts.emplace_back(Vec3(corners[2].x, corners[2].y, 0.f), color);
	verts.emplace_back(Vec3(corners[3].x, corners[3].y, 0.f), color);
}

void AddVertsForSector2D(
	std::vector<Vertex>& verts,
	Vec2                 sectorOrigin,
	float                sectorForwardDegrees,
	float                sectorApertureDegrees,
	float                sectorRadius,
	Rgba8                color
)
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
		float angle0 = startAngle + i * delta;
		float angle1 = startAngle + (i + 1) * delta;
		Vec2  p0     = sectorOrigin;
		Vec2  p1     = sectorOrigin + Vec2::MakeFromPolarDegrees(angle0, sectorRadius);
		Vec2  p2     = sectorOrigin + Vec2::MakeFromPolarDegrees(angle1, sectorRadius);
		verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
		verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
		verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);
	}
}

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Vec2 boneStart, Vec2 boneEnd, float radius, Rgba8 color)
{
	// Draw the body as a rectangle using AddVertsForLineSegment2D
	Vec2 dir          = (boneEnd - boneStart).GetNormalized();
	Vec2 right        = dir.GetRotatedByMinus90Degrees();
	Vec2 thicknessVec = right * (radius * 2.f);

	OBB2 capsuleBody             = OBB2();
	capsuleBody.m_center         = (boneStart + boneEnd) * 0.5f;
	capsuleBody.m_iBasisNormal   = dir;
	capsuleBody.m_halfDimensions = Vec2((boneEnd - boneStart).GetLength() * 0.5f, radius);

	AddVertsForOBB2D(verts, capsuleBody, color);

	// Draw the semicircle at each end using AddVertsForSector2D
	float forwardDegrees = dir.GetOrientationDegrees();
	AddVertsForSector2D(verts, boneStart, forwardDegrees + 180.f, 180.f, radius, color);
	AddVertsForSector2D(verts, boneEnd, forwardDegrees, 180.f, radius, color);
}

void AddVertsForTriangle2D(std::vector<Vertex>& verts, Vec2 ccw0, Vec2 ccw1, Vec2 ccw2, Rgba8 color)
{
	verts.emplace_back(Vec3(ccw0.x, ccw0.y, 0.f), color);
	verts.emplace_back(Vec3(ccw1.x, ccw1.y, 0.f), color);
	verts.emplace_back(Vec3(ccw2.x, ccw2.y, 0.f), color);
}

void AddVertsForLineSegment2D(std::vector<Vertex>& verts, Vec2 start, Vec2 end, Vec2 thickness, Rgba8 color)
{
	Vec2 dir       = (end - start).GetNormalized();
	Vec2 right     = dir.GetRotatedByMinus90Degrees();
	Vec2 halfThick = right * (thickness.x * 0.5f);

	Vec2 p0 = start + halfThick;
	Vec2 p1 = end + halfThick;
	Vec2 p2 = end - halfThick;
	Vec2 p3 = start - halfThick;

	verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
	verts.emplace_back(Vec3(p1.x, p1.y, 0.f), color);
	verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);

	verts.emplace_back(Vec3(p0.x, p0.y, 0.f), color);
	verts.emplace_back(Vec3(p2.x, p2.y, 0.f), color);
	verts.emplace_back(Vec3(p3.x, p3.y, 0.f), color);
}

void AddVertsForInfiniteLine2D(
	std::vector<Vertex>& verts, Vec2 pointOnLine, Vec2 anotherPointOnLine, float thickness, Rgba8 color
)
{
	Vec2 direction = anotherPointOnLine - pointOnLine;
	AddVertsForLineSegment2D(
		verts,
		pointOnLine - direction.GetNormalized() * 10000.f,
		pointOnLine + direction.GetNormalized() * 10000.f,
		Vec2(thickness, thickness),
		color
	);
}

void AddVertsForArrow2D(
	std::vector<Vertex>& verts, Vec2 tailPos, Vec2 tipPos, float arrowSize, float lineThickness, Rgba8 color
)
{
	Vec2  dir    = (tipPos - tailPos).GetNormalized();
	float length = (tipPos - tailPos).GetLength();

	float headLength  = (arrowSize > 0.f) ? std::min(arrowSize, length * 0.5f) : length * 0.2f;
	float shaftLength = length - headLength;

	Vec2 headBase = tipPos - dir * headLength;

	Vec2  right     = dir.GetRotatedByMinus90Degrees();
	Vec2  halfShaft = right * (lineThickness * 0.5f);
	float headWidth = headLength;
	Vec2  halfHead  = right * (headWidth * 0.5f);

	if (shaftLength > 0.0f)
	{
		Vec2 shaftStartL = tailPos + halfShaft;
		Vec2 shaftStartR = tailPos - halfShaft;
		Vec2 shaftEndL   = headBase + halfShaft;
		Vec2 shaftEndR   = headBase - halfShaft;

		verts.emplace_back(Vec3(shaftStartL.x, shaftStartL.y, 0.f), color);
		verts.emplace_back(Vec3(shaftEndL.x, shaftEndL.y, 0.f), color);
		verts.emplace_back(Vec3(shaftEndR.x, shaftEndR.y, 0.f), color);

		verts.emplace_back(Vec3(shaftStartL.x, shaftStartL.y, 0.f), color);
		verts.emplace_back(Vec3(shaftEndR.x, shaftEndR.y, 0.f), color);
		verts.emplace_back(Vec3(shaftStartR.x, shaftStartR.y, 0.f), color);
	}

	Vec2 headRight = headBase + halfHead;
	Vec2 headLeft  = headBase - halfHead;

	verts.emplace_back(Vec3(tipPos.x, tipPos.y, 0.f), color);
	verts.emplace_back(Vec3(headLeft.x, headLeft.y, 0.f), color);
	verts.emplace_back(Vec3(headRight.x, headRight.y, 0.f), color);
}

void AddVertsForQuad3D(
	std::vector<Vertex>& verts,
	const Vec3&          bottomLeft,
	const Vec3&          bottomRight,
	const Vec3&          topRight,
	const Vec3&          topLeft,
	const Rgba8&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit */
)
{
	Vec2 uvMins = UVs.m_mins;
	Vec2 uvMaxs = UVs.m_maxs;

	Vec3 const tangent   = (bottomRight - bottomLeft).GetNormalized();
	Vec3 const bitangent = (topLeft - bottomLeft).GetNormalized();
	Vec3 const normal    = CrossProduct3D(tangent, bitangent).GetNormalized();

	verts.emplace_back(bottomLeft, color, Vec2(uvMins.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(bottomRight, color, Vec2(uvMaxs.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(topRight, color, Vec2(uvMaxs.x, uvMaxs.y), tangent, bitangent, normal);

	verts.emplace_back(bottomLeft, color, Vec2(uvMins.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(topRight, color, Vec2(uvMaxs.x, uvMaxs.y), tangent, bitangent, normal);
	verts.emplace_back(topLeft, color, Vec2(uvMins.x, uvMaxs.y), tangent, bitangent, normal);
}

void AddVertsForQuad3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const Vec3&                bottomLeft,
	const Vec3&                bottomRight,
	const Vec3&                topRight,
	const Vec3&                topLeft,
	const Rgba8&               color /*= Rgba8::kWhite*/,
	const AABB2&               UVs /*= AABB2::Unit */
)
{
	Vec2 const uvMins = UVs.m_mins;
	Vec2 const uvMaxs = UVs.m_maxs;

	Vec3 const tangent   = (bottomRight - bottomLeft).GetNormalized();
	Vec3 const bitangent = (topLeft - bottomLeft).GetNormalized();
	Vec3 const normal    = CrossProduct3D(tangent, bitangent).GetNormalized();

	unsigned int startIndex = (unsigned int)verts.size();

	verts.emplace_back(bottomLeft, color, Vec2(uvMins.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(bottomRight, color, Vec2(uvMaxs.x, uvMins.y), tangent, bitangent, normal);
	verts.emplace_back(topRight, color, Vec2(uvMaxs.x, uvMaxs.y), tangent, bitangent, normal);
	verts.emplace_back(topLeft, color, Vec2(uvMins.x, uvMaxs.y), tangent, bitangent, normal);

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
	const Rgba8&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/
)
{
	Vec3 const& mins = bounds.m_mins;
	Vec3 const& maxs = bounds.m_maxs;

	// +Z (top)
	AddVertsForQuad3D(
		verts,
		Vec3(mins.x, mins.y, maxs.z),
		Vec3(maxs.x, mins.y, maxs.z),
		Vec3(maxs.x, maxs.y, maxs.z),
		Vec3(mins.x, maxs.y, maxs.z),
		color,
		UVs
	);

	// -Z (bottom)
	AddVertsForQuad3D(
		verts,
		Vec3(mins.x, maxs.y, mins.z),
		Vec3(maxs.x, maxs.y, mins.z),
		Vec3(maxs.x, mins.y, mins.z),
		Vec3(mins.x, mins.y, mins.z),
		color,
		UVs
	);

	// +X
	AddVertsForQuad3D(
		verts,
		Vec3(maxs.x, mins.y, mins.z),
		Vec3(maxs.x, maxs.y, mins.z),
		Vec3(maxs.x, maxs.y, maxs.z),
		Vec3(maxs.x, mins.y, maxs.z),
		color,
		UVs
	);

	// -X
	AddVertsForQuad3D(
		verts,
		Vec3(mins.x, maxs.y, mins.z),
		Vec3(mins.x, mins.y, mins.z),
		Vec3(mins.x, mins.y, maxs.z),
		Vec3(mins.x, maxs.y, maxs.z),
		color,
		UVs
	);

	// +Y
	AddVertsForQuad3D(
		verts,
		Vec3(maxs.x, maxs.y, mins.z),
		Vec3(mins.x, maxs.y, mins.z),
		Vec3(mins.x, maxs.y, maxs.z),
		Vec3(maxs.x, maxs.y, maxs.z),
		color,
		UVs
	);

	// -Y
	AddVertsForQuad3D(
		verts,
		Vec3(mins.x, mins.y, mins.z),
		Vec3(maxs.x, mins.y, mins.z),
		Vec3(maxs.x, mins.y, maxs.z),
		Vec3(mins.x, mins.y, maxs.z),
		color,
		UVs
	);
}

void AddVertsForAABB3D(
	std::vector<Vertex>&       verts,
	std::vector<unsigned int>& indexes,
	const AABB3&               bounds,
	const Rgba8&               color,
	const AABB2&               UVs
)
{
	Vec3 const& mins = bounds.m_mins;
	Vec3 const& maxs = bounds.m_maxs;

	// +Z (top)
	AddVertsForQuad3D(
		verts,
		indexes,
		Vec3(mins.x, mins.y, maxs.z),
		Vec3(maxs.x, mins.y, maxs.z),
		Vec3(maxs.x, maxs.y, maxs.z),
		Vec3(mins.x, maxs.y, maxs.z),
		color,
		UVs
	);

	// -Z (bottom)
	AddVertsForQuad3D(
		verts,
		indexes,
		Vec3(mins.x, maxs.y, mins.z),
		Vec3(maxs.x, maxs.y, mins.z),
		Vec3(maxs.x, mins.y, mins.z),
		Vec3(mins.x, mins.y, mins.z),
		color,
		UVs
	);

	// +X
	AddVertsForQuad3D(
		verts,
		indexes,
		Vec3(maxs.x, mins.y, mins.z),
		Vec3(maxs.x, maxs.y, mins.z),
		Vec3(maxs.x, maxs.y, maxs.z),
		Vec3(maxs.x, mins.y, maxs.z),
		color,
		UVs
	);

	// -X
	AddVertsForQuad3D(
		verts,
		indexes,
		Vec3(mins.x, maxs.y, mins.z),
		Vec3(mins.x, mins.y, mins.z),
		Vec3(mins.x, mins.y, maxs.z),
		Vec3(mins.x, maxs.y, maxs.z),
		color,
		UVs
	);

	// +Y
	AddVertsForQuad3D(
		verts,
		indexes,
		Vec3(maxs.x, maxs.y, mins.z),
		Vec3(mins.x, maxs.y, mins.z),
		Vec3(mins.x, maxs.y, maxs.z),
		Vec3(maxs.x, maxs.y, maxs.z),
		color,
		UVs
	);

	// -Y
	AddVertsForQuad3D(
		verts,
		indexes,
		Vec3(mins.x, mins.y, mins.z),
		Vec3(maxs.x, mins.y, mins.z),
		Vec3(maxs.x, mins.y, maxs.z),
		Vec3(mins.x, mins.y, maxs.z),
		color,
		UVs
	);
}

void AddVertsForSphere3D(
	std::vector<Vertex>& verts,
	const Vec3&          center,
	float                radius,
	const Rgba8&         color /*= Rgba8::kWhite*/,
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

	auto AddSphereVertex = [&](Vec3 const& position, float u, float v, float yaw)
	{
		Vec3 const normal    = (position - center).GetNormalized();
		Vec3 const tangent   = Vec3(-sinf(yaw), cosf(yaw), 0.f).GetNormalized();
		Vec3 const bitangent = CrossProduct3D(normal, tangent).GetNormalized();
		verts.emplace_back(position, color, Vec2(u, v), tangent, bitangent, normal);
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

			Vec3 p00 = center + Vec3::MakeFromPolarRadians(pitch0, yaw0, radius);
			Vec3 p10 = center + Vec3::MakeFromPolarRadians(pitch0, yaw1, radius);
			Vec3 p11 = center + Vec3::MakeFromPolarRadians(pitch1, yaw1, radius);
			Vec3 p01 = center + Vec3::MakeFromPolarRadians(pitch1, yaw0, radius);

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

void AddVertsForCylinder3D(
	std::vector<Vertex>& verts,
	const Vec3&          start,
	const Vec3&          end,
	float                radius,
	const Rgba8&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/,
	int                  numSlices /*= 16*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vec3  axis   = end - start;
	float height = axis.GetLength();
	if (height <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);

	Vec3 kBasis = axis / height;

	// Pick a helper not parallel to kBasis
	Vec3 helper = (Abs(kBasis.z) < 0.999f) ? Vec3(0.f, 0.f, 1.f) : Vec3(0.f, 1.f, 0.f);

	Vec3  iBasis = CrossProduct3D(helper, kBasis);
	float iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;

	Vec3 jBasis = CrossProduct3D(kBasis, iBasis); // already normalized if i,k are orthonormal

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

		Vec3 rim0 = (iBasis * cos0 + jBasis * sin0) * radius;
		Vec3 rim1 = (iBasis * cos1 + jBasis * sin1) * radius;

		Vec3 b0 = start + rim0;
		Vec3 b1 = start + rim1;
		Vec3 t0 = end + rim0;
		Vec3 t1 = end + rim1;

		Vec3 const radial0    = rim0.GetNormalized();
		Vec3 const radial1    = rim1.GetNormalized();
		Vec3 const tangent0   = (-iBasis * sin0 + jBasis * cos0).GetNormalized();
		Vec3 const tangent1   = (-iBasis * sin1 + jBasis * cos1).GetNormalized();
		Vec3 const sideBitangent = kBasis;

		float u0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);

		float u0 = UVs.m_mins.x + u0Frac * uRange;
		float u1 = UVs.m_mins.x + u1Frac * uRange;

		float vBottom = UVs.m_mins.y;
		float vTop    = UVs.m_mins.y + vRange;

		// Side quad (two triangles)
		verts.emplace_back(b0, color, Vec2(u0, vBottom), tangent0, sideBitangent, radial0);
		verts.emplace_back(b1, color, Vec2(u1, vBottom), tangent1, sideBitangent, radial1);
		verts.emplace_back(t1, color, Vec2(u1, vTop), tangent1, sideBitangent, radial1);

		verts.emplace_back(b0, color, Vec2(u0, vBottom), tangent0, sideBitangent, radial0);
		verts.emplace_back(t1, color, Vec2(u1, vTop), tangent1, sideBitangent, radial1);
		verts.emplace_back(t0, color, Vec2(u0, vTop), tangent0, sideBitangent, radial0);

		// Bottom cap (-kBasis) - triangle fan
		Vec3  cB  = start;
		float cu0 = UVs.m_mins.x + (0.5f + 0.5f * cos0) * uRange;
		float cv0 = UVs.m_mins.y + (0.5f + 0.5f * sin0) * vRange;
		float cu1 = UVs.m_mins.x + (0.5f + 0.5f * cos1) * uRange;
		float cv1 = UVs.m_mins.y + (0.5f + 0.5f * sin1) * vRange;
		float cuC = UVs.m_mins.x + 0.5f * uRange;
		float cvC = UVs.m_mins.y + 0.5f * vRange;

		// Winding chosen to face outward on bottom
		Vec3 const bottomNormal    = -kBasis;
		Vec3 const bottomTangent   = iBasis;
		Vec3 const bottomBitangent = -jBasis;
		verts.emplace_back(cB, color, Vec2(cuC, cvC), bottomTangent, bottomBitangent, bottomNormal);
		verts.emplace_back(b1, color, Vec2(cu1, cv1), bottomTangent, bottomBitangent, bottomNormal);
		verts.emplace_back(b0, color, Vec2(cu0, cv0), bottomTangent, bottomBitangent, bottomNormal);

		// Top cap (+kBasis)
		Vec3 cT = end;
		// Winding chosen to face outward on top
		verts.emplace_back(cT, color, Vec2(cuC, cvC), iBasis, jBasis, kBasis);
		verts.emplace_back(t0, color, Vec2(cu0, cv0), iBasis, jBasis, kBasis);
		verts.emplace_back(t1, color, Vec2(cu1, cv1), iBasis, jBasis, kBasis);
	}
}

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Vec3 const&          start,
	Vec3 const&          end,
	float                radius,
	AABB2 const&         UVs,
	Rgba8 const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vec3 const  axis       = end - start;
	float const axisLength = axis.GetLength();
	if (axisLength <= 0.f)
	{
		AddVertsForSphere3D(verts, start, radius, color, UVs, numSlices, numStacks);
		return;
	}

	numSlices = Max(3, numSlices);
	numStacks = Max(2, numStacks);

	Vec3 kBasis = axis / axisLength;

	Vec3  helper = (Abs(kBasis.z) < 0.999f) ? Vec3(0.f, 0.f, 1.f) : Vec3(0.f, 1.f, 0.f);
	Vec3  iBasis = CrossProduct3D(helper, kBasis);
	float iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;
	Vec3 jBasis = CrossProduct3D(kBasis, iBasis);

	float const deltaYaw   = kTwoPi / static_cast<float>(numSlices);
	int const   hemiStacks = Max(1, numStacks / 2);

	float const uRange       = UVs.m_maxs.x - UVs.m_mins.x;
	float const vRange       = UVs.m_maxs.y - UVs.m_mins.y;
	float const totalVLength = axisLength + 2.f * radius;
	float const bodyVMinFrac = radius / totalVLength;
	float const bodyVMaxFrac = (radius + axisLength) / totalVLength;

	auto GetRadialDir = [&](float yaw) -> Vec3 { return iBasis * cosf(yaw) + jBasis * sinf(yaw); };

	auto GetHemispherePoint = [&](Vec3 const& center, float yaw, float pitch) -> Vec3
	{
		float radialScale = cosf(pitch);
		return center + GetRadialDir(yaw) * (radius * radialScale) + kBasis * (radius * sinf(pitch));
	};

	auto GetU = [&](float yawFrac) -> float { return UVs.m_mins.x + yawFrac * uRange; };

	auto GetV = [&](float vFrac) -> float { return UVs.m_mins.y + vFrac * vRange; };

	auto AddCapsuleVertex = [&](Vec3 const& position, Vec3 const& normal, float yaw, float u, float v)
	{
		Vec3 const tangent = (-iBasis * sinf(yaw) + jBasis * cosf(yaw)).GetNormalized();
		Vec3 const bitangent = CrossProduct3D(normal, tangent).GetNormalized();
		verts.emplace_back(position, color, Vec2(u, v), tangent, bitangent, normal);
	};

	for (int sliceIndex = 0; sliceIndex < numSlices; ++sliceIndex)
	{
		float const yaw0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float const yaw1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);
		float const yaw0     = deltaYaw * static_cast<float>(sliceIndex);
		float const yaw1     = deltaYaw * static_cast<float>(sliceIndex + 1);

		Vec3 const rim0 = GetRadialDir(yaw0) * radius;
		Vec3 const rim1 = GetRadialDir(yaw1) * radius;

		Vec3 const b0 = start + rim0;
		Vec3 const b1 = start + rim1;
		Vec3 const t0 = end + rim0;
		Vec3 const t1 = end + rim1;
		Vec3 const normal0 = rim0.GetNormalized();
		Vec3 const normal1 = rim1.GetNormalized();

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

			Vec3 const lower00 = GetHemispherePoint(start, yaw0, lowerPitch0);
			Vec3 const lower10 = GetHemispherePoint(start, yaw1, lowerPitch0);
			Vec3 const lower11 = GetHemispherePoint(start, yaw1, lowerPitch1);
			Vec3 const lower01 = GetHemispherePoint(start, yaw0, lowerPitch1);

			AddCapsuleVertex(lower00, (lower00 - start).GetNormalized(), yaw0, u0, GetV(lowerV0Frac));
			AddCapsuleVertex(lower10, (lower10 - start).GetNormalized(), yaw1, u1, GetV(lowerV0Frac));
			AddCapsuleVertex(lower11, (lower11 - start).GetNormalized(), yaw1, u1, GetV(lowerV1Frac));

			AddCapsuleVertex(lower00, (lower00 - start).GetNormalized(), yaw0, u0, GetV(lowerV0Frac));
			AddCapsuleVertex(lower11, (lower11 - start).GetNormalized(), yaw1, u1, GetV(lowerV1Frac));
			AddCapsuleVertex(lower01, (lower01 - start).GetNormalized(), yaw0, u0, GetV(lowerV1Frac));

			Vec3 const upper00 = GetHemispherePoint(end, yaw0, upperPitch0);
			Vec3 const upper10 = GetHemispherePoint(end, yaw1, upperPitch0);
			Vec3 const upper11 = GetHemispherePoint(end, yaw1, upperPitch1);
			Vec3 const upper01 = GetHemispherePoint(end, yaw0, upperPitch1);

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
	Vec3 const&          start,
	Vec3 const&          end,
	float                radius,
	Rgba8 const&         color /*= Rgba8::kWhite*/,
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
	Rgba8 const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/,
	int                  numStacks /*= 8*/
)
{
	AddVertsForCapsule3D(verts, capsule.m_start, capsule.m_end, capsule.m_radius, UVs, color, numSlices, numStacks);
}

void AddVertsForCapsule3D(
	std::vector<Vertex>& verts,
	Capsule3 const&      capsule,
	Rgba8 const&         color /*= Rgba8::kWhite*/,
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
		numStacks
	);
}

void AddVertsForCone3D(
	std::vector<Vertex>& verts,
	const Vec3&          start,
	const Vec3&          end,
	float                radius,
	const Rgba8&         color /*= Rgba8::kWhite*/,
	const AABB2&         UVs /*= AABB2::Unit*/,
	int                  numSlices /*= 32*/
)
{
	if (radius <= 0.f)
	{
		return;
	}

	Vec3  axis   = end - start;
	float height = axis.GetLength();
	if (height <= 0.f)
	{
		return;
	}

	numSlices = Max(3, numSlices);

	Vec3 kBasis = axis / height;

	Vec3 helper = (Abs(kBasis.z) < 0.999f) ? Vec3(0.f, 0.f, 1.f) : Vec3(0.f, 1.f, 0.f);

	Vec3  iBasis = CrossProduct3D(helper, kBasis);
	float iLen   = iBasis.GetLength();
	if (iLen <= 0.f)
	{
		return;
	}
	iBasis /= iLen;

	Vec3 jBasis = CrossProduct3D(kBasis, iBasis);

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

		Vec3 rim0 = (iBasis * cos0 + jBasis * sin0) * radius;
		Vec3 rim1 = (iBasis * cos1 + jBasis * sin1) * radius;

		Vec3 b0  = start + rim0;
		Vec3 b1  = start + rim1;
		Vec3 tip = end;
		Vec3 tangent0 = (-iBasis * sin0 + jBasis * cos0).GetNormalized();
		Vec3 tangent1 = (-iBasis * sin1 + jBasis * cos1).GetNormalized();
		Vec3 bitangent0 = (kBasis * height - rim0).GetNormalized();
		Vec3 bitangent1 = (kBasis * height - rim1).GetNormalized();
		Vec3 normal0 = CrossProduct3D(tangent0, bitangent0).GetNormalized();
		Vec3 normal1 = CrossProduct3D(tangent1, bitangent1).GetNormalized();
		Vec3 tipTangent = (tangent0 + tangent1).GetNormalized();
		Vec3 tipBitangent = (bitangent0 + bitangent1).GetNormalized();
		Vec3 tipNormal = CrossProduct3D(tipTangent, tipBitangent).GetNormalized();

		float u0Frac = static_cast<float>(sliceIndex) / static_cast<float>(numSlices);
		float u1Frac = static_cast<float>(sliceIndex + 1) / static_cast<float>(numSlices);

		float u0 = UVs.m_mins.x + u0Frac * uRange;
		float u1 = UVs.m_mins.x + u1Frac * uRange;

		float vBase = UVs.m_mins.y;
		float vTip  = UVs.m_mins.y + vRange;

		// Side triangle
		verts.emplace_back(b0, color, Vec2(u0, vBase), tangent0, bitangent0, normal0);
		verts.emplace_back(b1, color, Vec2(u1, vBase), tangent1, bitangent1, normal1);
		verts.emplace_back(tip, color, Vec2((u0 + u1) * 0.5f, vTip), tipTangent, tipBitangent, tipNormal);

		// Base cap (faces outward opposite to +kBasis => -kBasis)
		Vec3 cB = start;

		float cu0 = UVs.m_mins.x + (0.5f + 0.5f * cos0) * uRange;
		float cv0 = UVs.m_mins.y + (0.5f + 0.5f * sin0) * vRange;
		float cu1 = UVs.m_mins.x + (0.5f + 0.5f * cos1) * uRange;
		float cv1 = UVs.m_mins.y + (0.5f + 0.5f * sin1) * vRange;
		float cuC = UVs.m_mins.x + 0.5f * uRange;
		float cvC = UVs.m_mins.y + 0.5f * vRange;

		Vec3 const baseNormal    = -kBasis;
		Vec3 const baseTangent   = iBasis;
		Vec3 const baseBitangent = -jBasis;
		verts.emplace_back(cB, color, Vec2(cuC, cvC), baseTangent, baseBitangent, baseNormal);
		verts.emplace_back(b1, color, Vec2(cu1, cv1), baseTangent, baseBitangent, baseNormal);
		verts.emplace_back(b0, color, Vec2(cu0, cv0), baseTangent, baseBitangent, baseNormal);
	}
}

void AddVertsForArrow3D(
	std::vector<Vertex>& verts,
	Vec3 const&          start,
	Vec3 const&          end,
	float                radius,
	Rgba8 const&         color /*= Rgba8::kWhite*/,
	int                  numSlices /*= 16*/
)
{
	Vec3  dir    = end - start;
	float length = dir.GetLength();
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

	Vec3  shaftEnd    = start + (dir / length) * shaftLength;
	float shaftRadius = radius * 0.30f;

	AddVertsForCylinder3D(verts, start, shaftEnd, shaftRadius, color, AABB2::Unit, numSlices);
	AddVertsForCone3D(verts, shaftEnd, end, radius, color, AABB2::Unit, numSlices);
}

void AddVertsForDisc2D(std::vector<Vertex>& verts, Disc2 const& disc, Rgba8 color)
{
	AddVertsForDisc2D(verts, disc.m_center, disc.m_radius, color);
}

void AddVertsForCapsule2D(std::vector<Vertex>& verts, Capsule2 const& capsule, Rgba8 color)
{
	AddVertsForCapsule2D(verts, capsule.m_bone.m_start, capsule.m_bone.m_end, capsule.m_radius, color);
}

void AddVertsForTriangle2D(std::vector<Vertex>& verts, Triangle2 const& triangle, Rgba8 color)
{
	AddVertsForTriangle2D(
		verts,
		triangle.m_pointsCounterClockwise[0],
		triangle.m_pointsCounterClockwise[1],
		triangle.m_pointsCounterClockwise[2],
		color
	);
}

void AddVertsForLineSegment2D(std::vector<Vertex>& verts, LineSegment2 const& lineSegment, float thickness, Rgba8 color)
{
	AddVertsForLineSegment2D(verts, lineSegment.m_start, lineSegment.m_end, Vec2(thickness, thickness), color);
}

void AddVertsForInfiniteLine2D(
	std::vector<Vertex>& verts, LineSegment2 const& infiniteLine, float thickness, Rgba8 color
)
{
	AddVertsForInfiniteLine2D(verts, infiniteLine.m_start, infiniteLine.m_end, thickness, color);
}

