#include "MingEngine/Core/Math/Disc2.hpp"

Disc2::Disc2(const Vector2& center, float radius) : m_center(center), m_radius(radius) {}

Disc2::Disc2(float centerX, float centerY, float radius) : m_center(centerX, centerY), m_radius(radius) {}

void Disc2::Translate(const Vector2& translation) { m_center += translation; }

void Disc2::SetCenter(const Vector2& newCenter) { m_center = newCenter; }
