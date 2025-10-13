#include "Camera.hpp"

#include "Engine/Core/Engine.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"

Camera::Camera(float left, float right, float bottom, float top) :
	m_leftBottom(left, bottom), m_rightTop(right, top),
	m_baseLeftBottom(left, bottom), m_baseRightTop(right, top)
{
}

void Camera::Update([[maybe_unused]] float deltaSeconds)
{
	if (m_shakeTimer > 0.f)
	{
		Shake();
		m_shakeTimer -= deltaSeconds;
		m_shakeTimer = Max(m_shakeTimer, 0.f);
	}
	else
	{
		m_leftBottom = m_baseLeftBottom;
		m_rightTop = m_baseRightTop;
	}
}

void Camera::SetOrthoView(Vec2 const& leftBottom, Vec2 const& rightTop)
{
	m_baseLeftBottom = leftBottom;
	m_baseRightTop = rightTop;
	
	m_leftBottom = m_baseLeftBottom;
	m_rightTop = m_baseRightTop;
}

void Camera::SetOrthoView(float left, float right, float bottom, float top)
{
	m_baseLeftBottom = Vec2(left, bottom);
	m_baseRightTop = Vec2(right, top);
	
	m_leftBottom = m_baseLeftBottom;
	m_rightTop = m_baseRightTop;
}

void Camera::SetPosition(Vec2 const& pos)
{
	Vec2 dimensions = GetDimensions();
	m_baseLeftBottom = pos - dimensions * 0.5f;
	m_baseRightTop = pos + dimensions * 0.5f;

	m_leftBottom = m_baseLeftBottom;
	m_rightTop = m_baseRightTop;
}

void Camera::TryShake(float shakeDuration, float shakeIntensity)
{
	m_shakeTimer = shakeDuration;
	m_shakeDuration = shakeDuration;
	m_shakeIntensity = shakeIntensity;
}

Vec2 Camera::GetDimensions() const
{
	return m_rightTop - m_leftBottom;
}

void Camera::Shake()
{
	if (m_shakeIntensity <= 0.f)
	{
		return;
	}

	float magnitude = SinDegrees(ConvertRadiansToDegrees((m_shakeTimer / m_shakeDuration) * HALF_PI)) * m_shakeIntensity;
	Vec2 shakeOffset = g_engine->m_rng->RollRandomVec2InRange(-magnitude, magnitude);
	
	m_leftBottom = m_baseLeftBottom + shakeOffset;
	m_rightTop = m_baseRightTop + shakeOffset;
}

