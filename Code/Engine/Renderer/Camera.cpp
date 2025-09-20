#include "Camera.hpp"

Camera::Camera(float left, float right, float bottom, float top) : m_leftBottom(left, bottom)
, m_rightTop(right, top)
{

}

void Camera::SetOrthoView(Vec2 const& leftBottom, Vec2 const& rightTop)
{
	m_leftBottom = leftBottom;
	m_rightTop = rightTop;
}

void Camera::SetOrthoView(float left, float right, float bottom, float top)
{
	m_leftBottom = Vec2(left, bottom);
	m_rightTop = Vec2(right, top);
}

void Camera::SetPosition(Vec2 const& pos)
{
	Vec2 dimensions = GetDimensions();
	m_leftBottom = pos - dimensions * 0.5f;
	m_rightTop = pos + dimensions;
}

Vec2 Camera::GetDimensions() const
{
	return m_rightTop - m_leftBottom;
}

