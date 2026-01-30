#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/MathUtils.hpp"

EulerAngles::EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees)
    : m_yawDegrees(yawDegrees)
    , m_pitchDegrees(pitchDegrees)
    , m_rollDegrees(rollDegrees)
{}

Vec3 EulerAngles::GetForwardDir_IFwd_JLeft_KUp() const
{
    float cy = CosDegrees(m_yawDegrees);
    float sy = SinDegrees(m_yawDegrees);
    float cp = CosDegrees(m_pitchDegrees);
    float sp = SinDegrees(m_pitchDegrees);

    return Vec3(
        cp * cy,
        cp * sy,
        -sp
    );
}

Vec3 EulerAngles::GetForwardDir_IRight_JUp_KFwd() const
{
    float cy = CosDegrees(m_yawDegrees);
    float sy = SinDegrees(m_yawDegrees);
    float cp = CosDegrees(m_pitchDegrees);
    float sp = SinDegrees(m_pitchDegrees);
    float cr = CosDegrees(m_rollDegrees);
    float sr = SinDegrees(m_rollDegrees);

    return Vec3(
        cr * cy * sp + sr * sy,
        cr * sy * sp - sr * cy,
        cp * cr
    );
}

void EulerAngles::GetAsVectors_IFwd_JLeft_KUp(Vec3& out_forwardIBasis, Vec3& out_leftJBasis, Vec3& out_upKBasis) const
{
    float cy = CosDegrees(m_yawDegrees);
    float sy = SinDegrees(m_yawDegrees);
    float cp = CosDegrees(m_pitchDegrees);
    float sp = SinDegrees(m_pitchDegrees);
    float cr = CosDegrees(m_rollDegrees);
    float sr = SinDegrees(m_rollDegrees);

    out_forwardIBasis = Vec3(
        cp * cy,
        cp * sy,
        -sp
    );

    out_leftJBasis = Vec3(
        cy * sp * sr - cr * sy,
        sy * sp * sr + cr * cy,
        cp * sr
    );

    out_upKBasis = Vec3(
        cr * cy * sp + sr * sy,
        cr * sy * sp - sr * cy,
        cp * cr
    );
}

void EulerAngles::GetAsVectors_IRight_JUp_KFwd(Vec3& out_rightIBasis, Vec3& out_upJBasis, Vec3& out_forwardKBasis) const
{
    float cy = CosDegrees(m_yawDegrees);
    float sy = SinDegrees(m_yawDegrees);
    float cp = CosDegrees(m_pitchDegrees);
    float sp = SinDegrees(m_pitchDegrees);
    float cr = CosDegrees(m_rollDegrees);
    float sr = SinDegrees(m_rollDegrees);

    out_rightIBasis = Vec3(
        cp * cy,
        cp * sy,
        -sp
    );

    out_upJBasis = Vec3(
        cy * sp * sr - cr * sy,
        sy * sp * sr + cr * cy,
        cp * sr
    );

    out_forwardKBasis = Vec3(
        cr * cy * sp + sr * sy,
        cr * sy * sp - sr * cy,
        cp * cr
    );
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

void EulerAngles::operator+=(EulerAngles const& anglesToAdd)
{
    m_yawDegrees += anglesToAdd.m_yawDegrees;
    m_pitchDegrees += anglesToAdd.m_pitchDegrees;
    m_rollDegrees += anglesToAdd.m_rollDegrees;
}

EulerAngles const Interpolate(EulerAngles const& from, EulerAngles const& to, float lerpFraction)
{
    EulerAngles result;
    
    float yawDelta = GetShortestAngularDispDegrees(from.m_yawDegrees, to.m_yawDegrees);
    float pitchDelta = GetShortestAngularDispDegrees(from.m_pitchDegrees, to.m_pitchDegrees);
    float rollDelta = GetShortestAngularDispDegrees(from.m_rollDegrees, to.m_rollDegrees);

    result.m_yawDegrees = from.m_yawDegrees + yawDelta * lerpFraction;
    result.m_pitchDegrees = from.m_pitchDegrees + pitchDelta * lerpFraction;
    result.m_rollDegrees = from.m_rollDegrees + rollDelta * lerpFraction;

    return result;
}
