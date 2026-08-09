#include "MingEngine/Core/Math/Capsule2.hpp"
#include "MingEngine/Core/Math/MathUtils.hpp"

Capsule2::Capsule2(const Vector2& start, const Vector2& end, float radius) : m_bone(start, end), m_radius(radius) {}

void Capsule2::Translate(const Vector2& translation) { m_bone.Translate(translation); }

void Capsule2::SetCenter(const Vector2& newCenter) { m_bone.SetCenter(newCenter); }

void Capsule2::RotateAboutCenter(float rotationDeltaDegrees) { m_bone.RotateAboutCenter(rotationDeltaDegrees); }
