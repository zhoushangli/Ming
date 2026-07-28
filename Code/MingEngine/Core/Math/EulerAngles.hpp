#pragma once

#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

struct EulerAngles
{
public:
	EulerAngles() = default;
	EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees);
	static EulerAngles MakeFromForward(Vec3 const& forward);
	static EulerAngles Interpolate(EulerAngles const& start, EulerAngles const& end, float fraction);
	static EulerAngles InterpolateClamped(EulerAngles const& start, EulerAngles const& end, float fraction);

	Vec3      GetForwardDir_IFwd_JLeft_KUp() const;
	Vec3      GetForwardDir_IRight_JUp_KFwd() const;
	void      GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const;
	void      GetAsVectors_IRight_JUp_KFwd(Vec3& out_rightIBasis, Vec3& out_upJBasis, Vec3& out_forwardKBasis) const;
	Matrix4x4 GetAsMatrix_IFwd_JLeft_KUp() const;
	Matrix4x4 GetAsMatrix_IRight_JUp_KFwd() const;

	void SetFromMatrix_IFwd_JLeft_KUp(Matrix4x4 const& mat);
	void SetForwardDir_IFwd(Vec3 const& forwardIBasis);

	EulerAngles operator+(EulerAngles const& anglesToAdd) const;
	EulerAngles operator-(EulerAngles const& anglesToSubtract) const;
	EulerAngles operator*(float uniformScale) const;
	void        operator*=(float uniformScale);
	void        operator+=(EulerAngles const& anglesToAdd);
	bool		operator==(EulerAngles const& compare) const;
	bool		operator!=(EulerAngles const& compare) const;

	static const EulerAngles Zero;

public:
	float m_yawDegrees   = 0.f;
	float m_pitchDegrees = 0.f;
	float m_rollDegrees  = 0.f;
};

