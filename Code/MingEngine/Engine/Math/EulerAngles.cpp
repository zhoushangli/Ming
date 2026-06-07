#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"
#include "EulerAngles.hpp"

const EulerAngles EulerAngles::Zero = EulerAngles(0.f, 0.f, 0.f);

EulerAngles::EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees)
	: m_yawDegrees(yawDegrees), m_pitchDegrees(pitchDegrees), m_rollDegrees(rollDegrees)
{
}

EulerAngles EulerAngles::MakeFromForward(Vec3 const& forward)
{
	EulerAngles eulerAngles;
	eulerAngles.SetForwardDir_IFwd(forward);
	return eulerAngles;
}

Vec3 EulerAngles::GetForwardDir_IFwd_JLeft_KUp() const
{
	float cy = CosDegrees(m_yawDegrees);
	float sy = SinDegrees(m_yawDegrees);
	float cp = CosDegrees(m_pitchDegrees);
	float sp = SinDegrees(m_pitchDegrees);

	return Vec3(cp * cy, cp * sy, -sp);
}

Vec3 EulerAngles::GetForwardDir_IRight_JUp_KFwd() const
{
	float cy = CosDegrees(m_yawDegrees);
	float sy = SinDegrees(m_yawDegrees);
	float cp = CosDegrees(m_pitchDegrees);
	float sp = SinDegrees(m_pitchDegrees);
	float cr = CosDegrees(m_rollDegrees);
	float sr = SinDegrees(m_rollDegrees);

	return Vec3(cr * cy * sp + sr * sy, cr * sy * sp - sr * cy, cp * cr);
}

void EulerAngles::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const
{
	float cy = CosDegrees(m_yawDegrees);
	float sy = SinDegrees(m_yawDegrees);
	float cp = CosDegrees(m_pitchDegrees);
	float sp = SinDegrees(m_pitchDegrees);
	float cr = CosDegrees(m_rollDegrees);
	float sr = SinDegrees(m_rollDegrees);

	out_forwardIBasis = Vec3(cp * cy, cp * sy, -sp);

	out_leftJBasis = Vec3(cy * sp * sr - cr * sy, sy * sp * sr + cr * cy, cp * sr);

	out_upKBasis = Vec3(cr * cy * sp + sr * sy, cr * sy * sp - sr * cy, cp * cr);
}

void EulerAngles::GetAsVectors_IRight_JUp_KFwd(Vec3& out_rightIBasis, Vec3& out_upJBasis, Vec3& out_forwardKBasis) const
{
	float cy = CosDegrees(m_yawDegrees);
	float sy = SinDegrees(m_yawDegrees);
	float cp = CosDegrees(m_pitchDegrees);
	float sp = SinDegrees(m_pitchDegrees);
	float cr = CosDegrees(m_rollDegrees);
	float sr = SinDegrees(m_rollDegrees);

	out_rightIBasis = Vec3(cp * cy, cp * sy, -sp);

	out_upJBasis = Vec3(cy * sp * sr - cr * sy, sy * sp * sr + cr * cy, cp * sr);

	out_forwardKBasis = Vec3(cr * cy * sp + sr * sy, cr * sy * sp - sr * cy, cp * cr);
}

Matrix4x4 EulerAngles::GetAsMatrix_IFwd_JLeft_KUp() const
{
	Vec3 i, j, k;
	GetAsVectors_IFwd_JLeft_KUp(i, j, k);
	return Matrix4x4(i, j, k, Vec3(0.f, 0.f, 0.f));
}

Matrix4x4 EulerAngles::GetAsMatrix_IRight_JUp_KFwd() const
{
	Vec3 i, j, k;
	GetAsVectors_IRight_JUp_KFwd(i, j, k);
	return Matrix4x4(i, j, k, Vec3(0.f, 0.f, 0.f));
}

void EulerAngles::SetFromMatrix_IFwd_JLeft_KUp(Matrix4x4 const& mat)
{
	Vec3 i = mat.GetIBasis3D().GetNormalized(); // forward
	Vec3 j = mat.GetJBasis3D().GetNormalized(); // left
	Vec3 k = mat.GetKBasis3D().GetNormalized(); // up

	m_yawDegrees   = Atan2Degrees(i.y, i.x);
	m_pitchDegrees = Atan2Degrees(-i.z, i.GetLengthXY());
	m_rollDegrees  = Atan2Degrees(j.z, k.z);
}

void EulerAngles::SetForwardDir_IFwd(Vec3 const& forwardIBasis)
{
	Vec3 helperVector = Vec3::Up;
	if (DotProduct3D(forwardIBasis, Vec3::Up) > 0.99f)
	{
		helperVector = Vec3::Right;
	}

	Vec3 leftJBasis = CrossProduct3D(helperVector, forwardIBasis);
	leftJBasis.Normalize();

	Vec3 upZBasis = CrossProduct3D(forwardIBasis, leftJBasis);
	upZBasis.Normalize();

	Matrix4x4 mat = Matrix4x4(forwardIBasis, leftJBasis, upZBasis, Vec3::Zero);
	SetFromMatrix_IFwd_JLeft_KUp(mat);
}

EulerAngles EulerAngles::operator+(EulerAngles const& anglesToAdd) const
{
	EulerAngles result = *this;
	result.m_pitchDegrees += anglesToAdd.m_pitchDegrees;
	result.m_yawDegrees += anglesToAdd.m_yawDegrees;
	result.m_rollDegrees += anglesToAdd.m_rollDegrees;
	return result;
}

EulerAngles EulerAngles::operator-(EulerAngles const& anglesToSubtract) const
{
	EulerAngles result = *this;
	result.m_pitchDegrees -= anglesToSubtract.m_pitchDegrees;
	result.m_yawDegrees -= anglesToSubtract.m_yawDegrees;
	result.m_rollDegrees -= anglesToSubtract.m_rollDegrees;
	return result;
}

void EulerAngles::operator*=(float uniformScale)
{
	m_yawDegrees *= uniformScale;
	m_pitchDegrees *= uniformScale;
	m_rollDegrees *= uniformScale;
}

void EulerAngles::operator+=(EulerAngles const& anglesToAdd)
{
	m_yawDegrees += anglesToAdd.m_yawDegrees;
	m_pitchDegrees += anglesToAdd.m_pitchDegrees;
	m_rollDegrees += anglesToAdd.m_rollDegrees;
}

EulerAngles EulerAngles::operator*(float uniformScale) const
{
	return EulerAngles(m_yawDegrees * uniformScale, m_pitchDegrees * uniformScale, m_rollDegrees * uniformScale);
}
