#pragma once

#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Matrix4x4.hpp"

struct EulerAngles
{
public:
    EulerAngles() = default;
    EulerAngles(float yawDegrees, float pitchDegrees, float rollDegrees);
    Vec3 GetForwardDir_IFwd_JLeft_KUp() const;
    Vec3 GetForwardDir_IRight_JUp_KFwd() const;
    void GetAsVectors_IFwd_JLeft_KUp(Vec3 &out_forwardIBasis, Vec3 &out_leftJBasis, Vec3 &out_upKBasis) const;
    void GetAsVectors_IRight_JUp_KFwd(Vec3 &out_rightIBasis, Vec3 &out_upJBasis, Vec3 &out_forwardKBasis) const;
    Matrix4x4 GetAsMatrix_IFwd_JLeft_KUp() const;
    Matrix4x4 GetAsMatrix_IRight_JUp_KFwd() const;

    void operator+=(EulerAngles const &anglesToAdd);

    friend EulerAngles const Interpolate(EulerAngles const &from, EulerAngles const &to, float lerpFraction);

    static const EulerAngles ZERO;

public:
    float m_yawDegrees = 0.f;
    float m_pitchDegrees = 0.f;
    float m_rollDegrees = 0.f;
};
