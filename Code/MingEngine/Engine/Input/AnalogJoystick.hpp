#pragma once

#include "MingEngine/Core/Math/Vector2.hpp"

class AnalogJoystick
{
public:
	Vector2 GetPosition() const;
	float   GetMagnitude() const;
	float   GetOrientationDegrees() const;

	Vector2 GetRawUncorrectedPosition() const;
	float   GetInnerDeadZoneFraction() const;
	float   GetOuterDeadZoneFraction() const;

	// For use by XboxController, et al.
	void Reset();
	void SetDeadZoneThresholds(float normalizedInnerDeadzoneThreshold, float normalizedOuterDeadzoneThreshold);
	void UpdatePosition(float rawNormalizedX, float rawNormalizedY);

protected:
	Vector2 m_rawPosition;                   // Flaky; doesn't rest at zero (or consistently snap to rest position)
	Vector2 m_correctedPosition;             // Deadzone-corrected position
	float   m_innerDeadZoneFraction = 0.3f;  // If R < this%, R = 0; "input range start" for corrective range map
	float   m_outerDeadZoneFraction = 0.95f; // if R > this%, R = 1; "input range end" for corrective range map
};
