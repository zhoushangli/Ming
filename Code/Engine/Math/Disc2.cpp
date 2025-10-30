#include "Engine/Math/Disc2.hpp"

void Disc2::Translate(const Vec2& translation)
{
	m_center += translation;
}

void Disc2::SetCenter(const Vec2& newCenter)
{
	m_center = newCenter;
}
