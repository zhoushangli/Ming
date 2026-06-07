#include "MingEngine/Engine/Math/OBB2.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"

OBB2::OBB2(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions) : 
    m_center(center),
    m_iBasisNormal(iBasisNormal),
	m_halfDimensions(halfDimensions)
{

}

OBB2::OBB2(Vec2 const& center, Vec2 const& halfDimensions, float orientationDegree) : 
    m_center(center),
    m_iBasisNormal(Vec2::MakeFromPolarDegrees(orientationDegree, 1.f)),
	m_halfDimensions(halfDimensions)
{
}

void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
    Vec2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();

	out_fourCornerWorldPositions[0] = m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[1] = m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[2] = m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;
	out_fourCornerWorldPositions[3] = m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y;
}

Vec2 OBB2::GetLocalPosForWorldPos(Vec2 const& worldPos) const
{
    Vec2 disp = worldPos - m_center;
    Vec2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();
    return Vec2(DotProduct2D(disp, m_iBasisNormal), DotProduct2D(disp, jBasisNormal));
}

Vec2 OBB2::GetWorldPosForLocalPos(Vec2 const& localPos) const
{
    Vec2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();
    return m_center + m_iBasisNormal * localPos.x + jBasisNormal * localPos.y;
}

void OBB2::RotateAboutCenter(float rotationDeltaDegrees)
{
    m_iBasisNormal = m_iBasisNormal.GetRotatedByDegrees(rotationDeltaDegrees);
}


