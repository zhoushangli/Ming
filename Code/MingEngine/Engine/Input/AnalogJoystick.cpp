#include "MingEngine/Engine/Input/AnalogJoystick.hpp"

#include "MingEngine/Core/Math/MathUtils.hpp"

Vec2 AnalogJoystick::GetPosition() const { return m_correctedPosition; }

float AnalogJoystick::GetMagnitude() const { return m_correctedPosition.GetLength(); }

float AnalogJoystick::GetOrientationDegrees() const { return m_correctedPosition.GetOrientationDegrees(); }

Vec2 AnalogJoystick::GetRawUncorrectedPosition() const { return m_rawPosition; }

float AnalogJoystick::GetInnerDeadZoneFraction() const { return m_innerDeadZoneFraction; }

float AnalogJoystick::GetOuterDeadZoneFraction() const { return m_outerDeadZoneFraction; }

void AnalogJoystick::Reset()
{
	m_rawPosition       = Vec2::Zero;
	m_correctedPosition = Vec2::Zero;
}

void AnalogJoystick::SetDeadZoneThresholds(
	float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold
)
{
	m_innerDeadZoneFraction = normalizedInnerDeadzoneThreshold;
	m_outerDeadZoneFraction = normalizedOuterDeadzoneThreshold;
}

void AnalogJoystick::UpdatePosition(float rawNormalizedX, float rawNormalizedY)
{
	m_rawPosition = Vec2(rawNormalizedX, rawNormalizedY);

	float magnitude = m_rawPosition.GetLength();

	if (magnitude < m_innerDeadZoneFraction)
	{
		m_correctedPosition = Vec2::Zero;
		return;
	}

	float correctedMagnitude = RangeMapClamped(magnitude, m_innerDeadZoneFraction, m_outerDeadZoneFraction, 0.f, 1.f);

	Vec2 direction      = (magnitude > 0.0f) ? m_rawPosition.GetNormalized() : Vec2::Zero;
	m_correctedPosition = direction * correctedMagnitude;
}

