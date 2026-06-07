#include "MingEngine/Scene/3D/Transform3D.hpp"

#include "MingEngine/Engine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Math/MathUtils.hpp"

namespace
{
constexpr float kScaleEpsilon = 1e-5f;

void GuaranteeScaleIsInvertible(Vec3 const& scale)
{
	GUARANTEE_OR_DIE(
		Abs(scale.x) > kScaleEpsilon && Abs(scale.y) > kScaleEpsilon && Abs(scale.z) > kScaleEpsilon,
		"Transform3D scale must be non-zero to build an inverse matrix."
	);
}
} // namespace

Matrix4x4 Transform3D::GetMatrix() const
{
	if (m_isMatrixDirty)
	{
		m_matrix = Matrix4x4::Identity;
		m_matrix.SetTranslation3D(m_position);
		m_matrix.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());
		m_matrix.Append(Matrix4x4::MakeNonUniformScale3D(m_scale));
		m_isMatrixDirty = false;
	}

	return m_matrix;
}

Matrix4x4 Transform3D::GetInverseMatrix() const
{
	if (m_isInverseMatrixDirty)
	{
		GuaranteeScaleIsInvertible(m_scale);

		Vec3 const inverseScale = Vec3(1.f / m_scale.x, 1.f / m_scale.y, 1.f / m_scale.z);

		Matrix4x4 inverseRotation = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
		inverseRotation.Transpose();

		m_inverseMatrix = Matrix4x4::Identity;
		m_inverseMatrix.Append(Matrix4x4::MakeNonUniformScale3D(inverseScale));
		m_inverseMatrix.Append(inverseRotation);
		m_inverseMatrix.Append(Matrix4x4::MakeTranslation3D(-m_position));
		m_isInverseMatrixDirty = false;
	}

	return m_inverseMatrix;
}

void Transform3D::SetMatrix(Matrix4x4 const& transform)
{
	Vec3 iBasis = transform.GetIBasis3D();
	Vec3 jBasis = transform.GetJBasis3D();
	Vec3 kBasis = transform.GetKBasis3D();

	m_position = transform.GetTranslation3D();
	m_scale    = Vec3(iBasis.GetLength(), jBasis.GetLength(), kBasis.GetLength());

	GuaranteeScaleIsInvertible(m_scale);

	iBasis /= m_scale.x;
	jBasis /= m_scale.y;
	kBasis /= m_scale.z;

	Matrix4x4 rotationMatrix(iBasis, jBasis, kBasis, Vec3::Zero);
	m_orientation.SetFromMatrix_IFwd_JLeft_KUp(rotationMatrix);

	m_matrix               = transform;
	m_isMatrixDirty        = false;
	m_isInverseMatrixDirty = true;
}

Vec3 Transform3D::GetPosition() const { return m_position; }

Vec3 Transform3D::GetScale() const { return m_scale; }

EulerAngles Transform3D::GetOrientation() const { return m_orientation; }

void Transform3D::SetPosition(Vec3 const& position)
{
	m_position = position;
	MarkDirty();
}

void Transform3D::SetScale(Vec3 const& scale)
{
	m_scale = scale;
	MarkDirty();
}

void Transform3D::SetOrientation(EulerAngles const& orientation)
{
	m_orientation = orientation;
	MarkDirty();
}

void Transform3D::MarkDirty()
{
	m_isMatrixDirty        = true;
	m_isInverseMatrixDirty = true;
}
