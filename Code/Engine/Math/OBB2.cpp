#include "Engine/Math/OBB2.hpp"
#include "Engine/Math/MathUtils.hpp"

void OBB2::GetCornerPoints(Vec2* out_fourCornerWorldPositions) const
{
    Vec2 jBasisNormal = m_iBasisNormal.GetRotatedBy90Degrees();
    Vec2 corners[4] = {
        m_center - m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y,
        m_center + m_iBasisNormal * m_halfDimensions.x - jBasisNormal * m_halfDimensions.y,
        m_center + m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y,
        m_center - m_iBasisNormal * m_halfDimensions.x + jBasisNormal * m_halfDimensions.y
    };
    for (int i = 0; i < 4; ++i) out_fourCornerWorldPositions[i] = corners[i];
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


