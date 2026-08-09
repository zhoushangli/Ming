#pragma once

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

struct EulerAngles
{
public:
	EulerAngles() = default;
	EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees);
	static EulerAngles MakeFromForward(Vector3 const& forward);
	static EulerAngles Interpolate(EulerAngles const& start, EulerAngles const& end, float fraction);
	static EulerAngles InterpolateClamped(EulerAngles const& start, EulerAngles const& end, float fraction);

	Vector3 GetForwardDir_IFwd_JLeft_KUp() const;
	Vector3 GetForwardDir_IRight_JUp_KFwd() const;
	void GetAsVectors_IFwd_JLeft_KUp(Vector3& out_forwardIBasis, Vector3& out_leftJBasis, Vector3& out_upKBasis) const;
	void GetAsVectors_IRight_JUp_KFwd(
		Vector3& out_rightIBasis, Vector3& out_upJBasis, Vector3& out_forwardKBasis) const;
	Matrix4x4 GetAsMatrix_IFwd_JLeft_KUp() const;
	Matrix4x4 GetAsMatrix_IRight_JUp_KFwd() const;

	void SetFromMatrix_IFwd_JLeft_KUp(Matrix4x4 const& mat);
	void SetForwardDir_IFwd(Vector3 const& forwardIBasis);

	EulerAngles operator+(EulerAngles const& anglesToAdd) const;
	EulerAngles operator-(EulerAngles const& anglesToSubtract) const;
	EulerAngles operator*(float uniformScale) const;
	void        operator*=(float uniformScale);
	void        operator+=(EulerAngles const& anglesToAdd);
	bool        operator==(EulerAngles const& compare) const;
	bool        operator!=(EulerAngles const& compare) const;

	static const EulerAngles Zero;

public:
	float m_yawDegrees   = 0.f;
	float m_pitchDegrees = 0.f;
	float m_rollDegrees  = 0.f;
};
