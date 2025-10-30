#include "Engine/Math/Capsule2.hpp"
#include "Engine/Math/MathUtils.hpp"

void Capsule2::Translate(const Vec2& translation)
{
    m_bone.Translate(translation);
}

void Capsule2::SetCenter(const Vec2& newCenter)
{
    m_bone.SetCenter(newCenter);
}

void Capsule2::RotateAboutCenter(float rotationDeltaDegrees)
{
    m_bone.RotateAboutCenter(rotationDeltaDegrees);
}
