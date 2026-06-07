#include "MingEngine/Engine/Math/Disc2.hpp"

Disc2::Disc2(const Vec2& center, float radius) : m_center(center), m_radius(radius)
{
}

Disc2::Disc2(float centerX, float centerY, float radius) : m_center(centerX, centerY), m_radius(radius)
{

}

void Disc2::Translate(const Vec2& translation)
{
	m_center += translation;
}

void Disc2::SetCenter(const Vec2& newCenter)
{
	m_center = newCenter;
}
