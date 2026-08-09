#include "MingEngine/Core/Math/OBB2.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"

using namespace Math;

OBB2::OBB2(Vector2 const& center, Vector2 const& iBasisNormal, Vector2 const& halfDimensions)
	: m_center(center), m_iBasisNormal(iBasisNormal), m_halfDimensions(halfDimensions)
{
}

OBB2::OBB2(Vector2 const& center, Vector2 const& halfDimensions, float orientationDegree)
	: m_center(center), m_iBasisNormal(Vector2::MakeFromPolarDegrees(orientationDegree, 1.f)),
	  m_halfDimensions(halfDimensions)
{
}

void OBB2::GetCornerPoints(Vector2* out_fourCornerWorldPositions) const
{
	Vector2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();

	out_fourCornerWorldPositions[0] =
		m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[1] =
		m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[2] =
		m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[3] =
		m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;
}

Vector2 OBB2::GetLocalPosForWorldPos(Vector2 const& worldPos) const
{
	Vector2 disp         = worldPos - m_center;
	Vector2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();
	return Vector2(DotProduct2D(disp, m_iBasisNormal), DotProduct2D(disp, jBasisNormal));
}

Vector2 OBB2::GetWorldPosForLocalPos(Vector2 const& localPos) const
{
	Vector2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();
	return m_center + m_iBasisNormal * localPos.x + jBasisNormal * localPos.y;
}

void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
	m_iBasisNormal = m_iBasisNormal.GetRotatedByDegrees(rotationDeltaDegrees);
}
