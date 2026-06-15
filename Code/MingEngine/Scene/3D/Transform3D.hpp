#pragma once

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

class Transform3D
{
public:
	Matrix4x4 GetMatrix() const;
	Matrix4x4 GetInverseMatrix() const;
	void      SetMatrix(Matrix4x4 const& transform);

	Vec3        GetPosition() const;
	Vec3        GetScale() const;
	EulerAngles GetOrientation() const;

	void SetPosition(Vec3 const& position);
	void SetScale(Vec3 const& scale);
	void SetOrientation(EulerAngles const& orientation);

private:
	void MarkDirty();

private:
	Vec3        m_position    = Vec3::Zero;
	Vec3        m_scale       = Vec3::One;
	EulerAngles m_orientation = EulerAngles::Zero;

	mutable Matrix4x4 m_matrix               = Matrix4x4::Identity;
	mutable Matrix4x4 m_inverseMatrix        = Matrix4x4::Identity;
	mutable bool      m_isMatrixDirty        = true;
	mutable bool      m_isInverseMatrixDirty = true;
};

