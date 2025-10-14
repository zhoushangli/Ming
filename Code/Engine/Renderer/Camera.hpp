#pragma once

#include "Engine/Math/Vec2.hpp"

class Game;

class Camera
{
public:
	Camera() = default;
	Camera(float left, float right, float bottom, float top);
	Camera(Camera const& copy) = default;
	~Camera() = default;

	void Update(float deltaSeconds);

	void SetOrthoView(Vec2 const& leftBottom, Vec2 const& rightTop);
	void SetOrthoView(float left, float right, float bottom, float top);
	void SetPosition(Vec2 const& pos);
	void TryShake(float shakeDuration, float shakeIntensity);

	Vec2 GetDimensions() const;

	float GetLeft() const { return m_leftBottom.x; }
	float GetRight() const { return m_rightTop.x; }
	float GetBottom() const { return m_leftBottom.y; }
	float GetTop() const { return m_rightTop.y; }

private:
	void Shake();

private:
	Vec2 m_leftBottom = Vec2::ZERO;
	Vec2 m_rightTop = Vec2::ZERO;

	Vec2 m_baseLeftBottom = Vec2::ZERO;
	Vec2 m_baseRightTop = Vec2::ZERO;
	
	float m_shakeDuration = 0.f;
	float m_shakeIntensity = 0.f;
	float m_shakeTimer = 0.f;
};

