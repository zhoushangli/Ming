#pragma once

#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vector3.hpp"

class Transform3D
{
public:
	Matrix4x4 GetMatrix() const;
	Matrix4x4 GetInverseMatrix() const;
	void      SetMatrix(Matrix4x4 const& transform);

	Vector3     GetPosition() const;
	Vector3     GetScale() const;
	EulerAngles GetOrientation() const;

	void SetPosition(Vector3 const& position);
	void SetScale(Vector3 const& scale);
	void SetOrientation(EulerAngles const& orientation);

private:
	void MarkDirty();

private:
	Vector3     m_position    = Vector3::Zero;
	Vector3     m_scale       = Vector3::One;
	EulerAngles m_orientation = EulerAngles::Zero;

	mutable Matrix4x4 m_matrix               = Matrix4x4::Identity;
	mutable Matrix4x4 m_inverseMatrix        = Matrix4x4::Identity;
	mutable bool      m_isMatrixDirty        = true;
	mutable bool      m_isInverseMatrixDirty = true;
};
