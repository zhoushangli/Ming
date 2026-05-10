#include "Engine/Math/Matrix4x4.hpp"

#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Math/Vec4.hpp"

Matrix4x4 const Matrix4x4::Zero = Matrix4x4(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

Matrix4x4 const Matrix4x4::Identity = Matrix4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

// Default constructor: Identity matrix
Matrix4x4::Matrix4x4()
{
	m_values[Ix] = 1.f;
	m_values[Jx] = 0.f;
	m_values[Kx] = 0.f;
	m_values[Tx] = 0.f;
	m_values[Iy] = 0.f;
	m_values[Jy] = 1.f;
	m_values[Ky] = 0.f;
	m_values[Ty] = 0.f;
	m_values[Iz] = 0.f;
	m_values[Jz] = 0.f;
	m_values[Kz] = 1.f;
	m_values[Tz] = 0.f;
	m_values[Iw] = 0.f;
	m_values[Jw] = 0.f;
	m_values[Kw] = 0.f;
	m_values[Tw] = 1.f;
}

// 2D basis and translation
Matrix4x4::Matrix4x4(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Jx] = jBasis2D.x;
	m_values[Kx] = 0.f;
	m_values[Tx] = translation2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Jy] = jBasis2D.y;
	m_values[Ky] = 0.f;
	m_values[Ty] = translation2D.y;
	m_values[Iz] = 0.f;
	m_values[Jz] = 0.f;
	m_values[Kz] = 1.f;
	m_values[Tz] = 0.f;
	m_values[Iw] = 0.f;
	m_values[Jw] = 0.f;
	m_values[Kw] = 0.f;
	m_values[Tw] = 1.f;
}

// 3D basis and translation
Matrix4x4::Matrix4x4(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Jx] = jBasis3D.x;
	m_values[Kx] = kBasis3D.x;
	m_values[Tx] = translation3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Jy] = jBasis3D.y;
	m_values[Ky] = kBasis3D.y;
	m_values[Ty] = translation3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Jz] = jBasis3D.z;
	m_values[Kz] = kBasis3D.z;
	m_values[Tz] = translation3D.z;
	m_values[Iw] = 0.f;
	m_values[Jw] = 0.f;
	m_values[Kw] = 0.f;
	m_values[Tw] = 1.f;
}

// 2D basis (with 3 vectors) & 4D translation
Matrix4x4::Matrix4x4(Vec2 const& IBasis2D, Vec2 const& jBasis2D, Vec2 const& kBasis2D, Vec4 const& translation4D)
{
	m_values[Ix] = IBasis2D.x;
	m_values[Jx] = jBasis2D.x;
	m_values[Kx] = kBasis2D.x;
	m_values[Tx] = translation4D.x;
	m_values[Iy] = IBasis2D.y;
	m_values[Jy] = jBasis2D.y;
	m_values[Ky] = kBasis2D.y;
	m_values[Ty] = translation4D.y;
	m_values[Iz] = 0.f;
	m_values[Jz] = 0.f;
	m_values[Kz] = 0.f;
	m_values[Tz] = translation4D.z;
	m_values[Iw] = 0.f;
	m_values[Jw] = 0.f;
	m_values[Kw] = 0.f;
	m_values[Tw] = translation4D.w;
}

// 4D basis and translation
Matrix4x4::Matrix4x4(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Jx] = jBasis4D.x;
	m_values[Kx] = kBasis4D.x;
	m_values[Tx] = translation4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Jy] = jBasis4D.y;
	m_values[Ky] = kBasis4D.y;
	m_values[Ty] = translation4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Jz] = jBasis4D.z;
	m_values[Kz] = kBasis4D.z;
	m_values[Tz] = translation4D.z;
	m_values[Iw] = iBasis4D.w;
	m_values[Jw] = jBasis4D.w;
	m_values[Kw] = kBasis4D.w;
	m_values[Tw] = translation4D.w;
}

// From float array (basis-major)
Matrix4x4::Matrix4x4(float const* sixteenValuesBasisMajor)
{
	for (int i = 0; i < 16; ++i)
	{
		m_values[i] = sixteenValuesBasisMajor[i];
	}
}

Matrix4x4::Matrix4x4(
	float ix,
	float jx,
	float kx,
	float tx,
	float iy,
	float jy,
	float ky,
	float ty,
	float iz,
	float jz,
	float kz,
	float tz,
	float iw,
	float jw,
	float kw,
	float tw
)
{
	m_values[Ix] = ix;
	m_values[Iy] = iy;
	m_values[Iz] = iz;
	m_values[Iw] = iw;

	m_values[Jx] = jx;
	m_values[Jy] = jy;
	m_values[Jz] = jz;
	m_values[Jw] = jw;

	m_values[Kx] = kx;
	m_values[Ky] = ky;
	m_values[Kz] = kz;
	m_values[Kw] = kw;

	m_values[Tx] = tx;
	m_values[Ty] = ty;
	m_values[Tz] = tz;
	m_values[Tw] = tw;
}

float* Matrix4x4::GetAsFloatArray() { return m_values; }

float const* Matrix4x4::GetAsFloatArray() const { return m_values; }

Vec2 const Matrix4x4::GetIBasis2D() const { return Vec2(m_values[Ix], m_values[Iy]); }

Vec2 const Matrix4x4::GetJBasis2D() const { return Vec2(m_values[Jx], m_values[Jy]); }

Vec2 const Matrix4x4::GetTranslation2D() const { return Vec2(m_values[Tx], m_values[Ty]); }

Vec3 const Matrix4x4::GetIBasis3D() const { return Vec3(m_values[Ix], m_values[Iy], m_values[Iz]); }

Vec3 const Matrix4x4::GetJBasis3D() const { return Vec3(m_values[Jx], m_values[Jy], m_values[Jz]); }

Vec3 const Matrix4x4::GetKBasis3D() const { return Vec3(m_values[Kx], m_values[Ky], m_values[Kz]); }

Vec3 const Matrix4x4::GetTranslation3D() const { return Vec3(m_values[Tx], m_values[Ty], m_values[Tz]); }

Vec4 const Matrix4x4::GetIBasis4D() const { return Vec4(m_values[Ix], m_values[Iy], m_values[Iz], m_values[Iw]); }

Vec4 const Matrix4x4::GetJBasis4D() const { return Vec4(m_values[Jx], m_values[Jy], m_values[Jz], m_values[Jw]); }

Vec4 const Matrix4x4::GetKBasis4D() const { return Vec4(m_values[Kx], m_values[Ky], m_values[Kz], m_values[Kw]); }

Vec4 const Matrix4x4::GetTranslation4D() const { return Vec4(m_values[Tx], m_values[Ty], m_values[Tz], m_values[Tw]); }

void Matrix4x4::SetTranslation2D(Vec2 const translationXY)
{
	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Matrix4x4::SetTranslation3D(Vec3 const translationXYZ)
{
	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Matrix4x4::SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D)
{
	m_values[Ix] = iBasis2D.x;
	m_values[Iy] = iBasis2D.y;
	m_values[Iz] = 0.f;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis2D.x;
	m_values[Jy] = jBasis2D.y;
	m_values[Jz] = 0.f;
	m_values[Jw] = 0.f;
}

void Matrix4x4::SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY)
{
	SetIJ2D(iBasis2D, jBasis2D);

	m_values[Tx] = translationXY.x;
	m_values[Ty] = translationXY.y;
	m_values[Tz] = 0.f;
	m_values[Tw] = 1.f;
}

void Matrix4x4::SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D)
{
	m_values[Ix] = iBasis3D.x;
	m_values[Iy] = iBasis3D.y;
	m_values[Iz] = iBasis3D.z;
	m_values[Iw] = 0.f;

	m_values[Jx] = jBasis3D.x;
	m_values[Jy] = jBasis3D.y;
	m_values[Jz] = jBasis3D.z;
	m_values[Jw] = 0.f;

	m_values[Kx] = kBasis3D.x;
	m_values[Ky] = kBasis3D.y;
	m_values[Kz] = kBasis3D.z;
	m_values[Kw] = 0.f;
}

void Matrix4x4::SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ)
{
	SetIJK3D(iBasis3D, jBasis3D, kBasis3D);

	m_values[Tx] = translationXYZ.x;
	m_values[Ty] = translationXYZ.y;
	m_values[Tz] = translationXYZ.z;
	m_values[Tw] = 1.f;
}

void Matrix4x4::SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D)
{
	m_values[Ix] = iBasis4D.x;
	m_values[Iy] = iBasis4D.y;
	m_values[Iz] = iBasis4D.z;
	m_values[Iw] = iBasis4D.w;

	m_values[Jx] = jBasis4D.x;
	m_values[Jy] = jBasis4D.y;
	m_values[Jz] = jBasis4D.z;
	m_values[Jw] = jBasis4D.w;

	m_values[Kx] = kBasis4D.x;
	m_values[Ky] = kBasis4D.y;
	m_values[Kz] = kBasis4D.z;
	m_values[Kw] = kBasis4D.w;

	m_values[Tx] = translation4D.x;
	m_values[Ty] = translation4D.y;
	m_values[Tz] = translation4D.z;
	m_values[Tw] = translation4D.w;
}

Matrix4x4 const Matrix4x4::MakeTranslation2D(Vec2 const translation)
{
	Matrix4x4 mat;
	mat.m_values[Tx] = translation.x;
	mat.m_values[Ty] = translation.y;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeTranslation3D(Vec3 const translationXYZ)
{
	Matrix4x4 mat;
	mat.m_values[Tx] = translationXYZ.x;
	mat.m_values[Ty] = translationXYZ.y;
	mat.m_values[Tz] = translationXYZ.z;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeUniformScale2D(float uniformScaleXY)
{
	Matrix4x4 mat;
	mat.m_values[Ix] = uniformScaleXY;
	mat.m_values[Jy] = uniformScaleXY;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeUniformScale3D(float uniformScaleXYZ)
{
	Matrix4x4 mat;
	mat.m_values[Ix] = uniformScaleXYZ;
	mat.m_values[Jy] = uniformScaleXYZ;
	mat.m_values[Kz] = uniformScaleXYZ;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeNonUniformScale2D(Vec2 const nonUniformScaleXY)
{
	Matrix4x4 mat;
	mat.m_values[Ix] = nonUniformScaleXY.x;
	mat.m_values[Jy] = nonUniformScaleXY.y;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeNonUniformScale3D(Vec3 const nonUniformScaleXYZ)
{
	Matrix4x4 mat;
	mat.m_values[Ix] = nonUniformScaleXYZ.x;
	mat.m_values[Jy] = nonUniformScaleXYZ.y;
	mat.m_values[Kz] = nonUniformScaleXYZ.z;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeRotationDegrees2D(float rotationDegreesAboutZ)
{
	float c = CosDegrees(rotationDegreesAboutZ);
	float s = SinDegrees(rotationDegreesAboutZ);

	Matrix4x4 mat;
	mat.m_values[Ix] = c;
	mat.m_values[Iy] = s;
	mat.m_values[Jx] = -s;
	mat.m_values[Jy] = c;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeRotationDegreesX(float rotationDegreesAboutX)
{
	float c = CosDegrees(rotationDegreesAboutX);
	float s = SinDegrees(rotationDegreesAboutX);

	Matrix4x4 mat;
	mat.m_values[Jy] = c;
	mat.m_values[Jz] = s;
	mat.m_values[Ky] = -s;
	mat.m_values[Kz] = c;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeRotationDegreesY(float rotationDegreesAboutY)
{
	float c = CosDegrees(rotationDegreesAboutY);
	float s = SinDegrees(rotationDegreesAboutY);

	Matrix4x4 mat;
	mat.m_values[Ix] = c;
	mat.m_values[Iz] = -s;
	mat.m_values[Kx] = s;
	mat.m_values[Kz] = c;
	return mat;
}

Matrix4x4 const Matrix4x4::MakeRotationDegreesZ(float rotationDegreesAboutZ)
{
	float c = CosDegrees(rotationDegreesAboutZ);
	float s = SinDegrees(rotationDegreesAboutZ);

	Matrix4x4 mat;
	mat.m_values[Ix] = c;
	mat.m_values[Iy] = s;
	mat.m_values[Jx] = -s;
	mat.m_values[Jy] = c;
	return mat;
}

Matrix4x4 const
Matrix4x4::MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar)
{
	Matrix4x4 ortho;
	ortho.m_values[Ix] = 2.f / (right - left);
	ortho.m_values[Jy] = 2.f / (top - bottom);
	ortho.m_values[Kz] = 1.f / (zFar - zNear);
	ortho.m_values[Tx] = (left + right) / (left - right);
	ortho.m_values[Ty] = (bottom + top) / (bottom - top);
	ortho.m_values[Tz] = zNear / (zNear - zFar);
	return ortho;
}

Matrix4x4 const Matrix4x4::MakePerspectiveProjection(float fovYDegrees, float aspect, float zNear, float zFar)
{
	Matrix4x4 perspective;

	float c      = CosDegrees(fovYDegrees * 0.5f);
	float s      = SinDegrees(fovYDegrees * 0.5f);
	float scaleY = c / s;
	float scaleX = scaleY / aspect;

	float scaleZ     = zFar / (zFar - zNear);
	float translateZ = (zNear * zFar) / (zNear - zFar);

	perspective.m_values[Ix] = scaleX;
	perspective.m_values[Jy] = scaleY;
	perspective.m_values[Kz] = scaleZ;
	perspective.m_values[Kw] = 1.f;
	perspective.m_values[Tz] = translateZ;
	perspective.m_values[Tw] = 0.f;

	return perspective;
}

Vec2 const Matrix4x4::TransformVectorQuantity2D(Vec2 const vectorQuantityXY) const
{
	float x = m_values[Ix] * vectorQuantityXY.x + m_values[Jx] * vectorQuantityXY.y;
	float y = m_values[Iy] * vectorQuantityXY.x + m_values[Jy] * vectorQuantityXY.y;
	return Vec2(x, y);
}

Vec3 const Matrix4x4::TransformVectorQuantity3D(Vec3 const vectorQuantityXYZ) const
{
	float x =
		m_values[Ix] * vectorQuantityXYZ.x + m_values[Jx] * vectorQuantityXYZ.y + m_values[Kx] * vectorQuantityXYZ.z;
	float y =
		m_values[Iy] * vectorQuantityXYZ.x + m_values[Jy] * vectorQuantityXYZ.y + m_values[Ky] * vectorQuantityXYZ.z;
	float z =
		m_values[Iz] * vectorQuantityXYZ.x + m_values[Jz] * vectorQuantityXYZ.y + m_values[Kz] * vectorQuantityXYZ.z;
	return Vec3(x, y, z);
}

Vec2 const Matrix4x4::TransformPosition2D(Vec2 const positionXY) const
{
	float x = m_values[Ix] * positionXY.x + m_values[Jx] * positionXY.y + m_values[Tx];
	float y = m_values[Iy] * positionXY.x + m_values[Jy] * positionXY.y + m_values[Ty];
	return Vec2(x, y);
}

Vec3 const Matrix4x4::TransformPosition3D(Vec3 const position3D) const
{
	float x = m_values[Ix] * position3D.x + m_values[Jx] * position3D.y + m_values[Kx] * position3D.z + m_values[Tx];
	float y = m_values[Iy] * position3D.x + m_values[Jy] * position3D.y + m_values[Ky] * position3D.z + m_values[Ty];
	float z = m_values[Iz] * position3D.x + m_values[Jz] * position3D.y + m_values[Kz] * position3D.z + m_values[Tz];
	return Vec3(x, y, z);
}

Vec3 const Matrix4x4::TransformDirection3D(Vec3 const direction3D) const
{
	float x = m_values[Ix] * direction3D.x + m_values[Jx] * direction3D.y + m_values[Kx] * direction3D.z;
	float y = m_values[Iy] * direction3D.x + m_values[Jy] * direction3D.y + m_values[Ky] * direction3D.z;
	float z = m_values[Iz] * direction3D.x + m_values[Jz] * direction3D.y + m_values[Kz] * direction3D.z;
	return Vec3(x, y, z);
}

Vec4 const Matrix4x4::TransformHomogeneous3D(Vec4 const homogeneous3D) const
{
	float x = m_values[Ix] * homogeneous3D.x + m_values[Jx] * homogeneous3D.y + m_values[Kx] * homogeneous3D.z
			  + m_values[Tx] * homogeneous3D.w;
	float y = m_values[Iy] * homogeneous3D.x + m_values[Jy] * homogeneous3D.y + m_values[Ky] * homogeneous3D.z
			  + m_values[Ty] * homogeneous3D.w;
	float z = m_values[Iz] * homogeneous3D.x + m_values[Jz] * homogeneous3D.y + m_values[Kz] * homogeneous3D.z
			  + m_values[Tz] * homogeneous3D.w;
	float w = m_values[Iw] * homogeneous3D.x + m_values[Jw] * homogeneous3D.y + m_values[Kw] * homogeneous3D.z
			  + m_values[Tw] * homogeneous3D.w;
	return Vec4(x, y, z, w);
}

void Matrix4x4::Append(Matrix4x4 const appendThis)
{
	Matrix4x4 result;
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			result.m_values[row * 4 + col] = m_values[0 * 4 + col] * appendThis.m_values[row * 4 + 0]
											 + m_values[1 * 4 + col] * appendThis.m_values[row * 4 + 1]
											 + m_values[2 * 4 + col] * appendThis.m_values[row * 4 + 2]
											 + m_values[3 * 4 + col] * appendThis.m_values[row * 4 + 3];
		}
	}
	for (int i = 0; i < 16; ++i)
	{
		m_values[i] = result.m_values[i];
	}
}

void Matrix4x4::AppendRotationX(float degreesAboutX)
{
	float c = CosDegrees(degreesAboutX);
	float s = SinDegrees(degreesAboutX);

	Matrix4x4 rot;

	rot.m_values[Jy] = c;
	rot.m_values[Jz] = s;
	rot.m_values[Ky] = -s;
	rot.m_values[Kz] = c;

	Append(rot);
}

void Matrix4x4::AppendRotationY(float degreesAboutY)
{
	float c = CosDegrees(degreesAboutY);
	float s = SinDegrees(degreesAboutY);

	Matrix4x4 rot;

	rot.m_values[Ix] = c;
	rot.m_values[Iz] = -s;

	rot.m_values[Kx] = s;
	rot.m_values[Kz] = c;

	Append(rot);
}

void Matrix4x4::AppendRotationZ(float degreesAboutZ)
{
	float c = CosDegrees(degreesAboutZ);
	float s = SinDegrees(degreesAboutZ);

	Matrix4x4 rot;

	rot.m_values[Ix] = c;
	rot.m_values[Iy] = s;

	rot.m_values[Jx] = -s;
	rot.m_values[Jy] = c;

	Append(rot);
}

void Matrix4x4::AppendTranslation2D(Vec2 const translationXY)
{
	Matrix4x4 trans;

	trans.m_values[Tx] = translationXY.x;
	trans.m_values[Ty] = translationXY.y;

	Append(trans);
}

void Matrix4x4::AppendTranslation3D(Vec3 const translationXYZ)
{
	Matrix4x4 trans;

	trans.m_values[Tx] = translationXYZ.x;
	trans.m_values[Ty] = translationXYZ.y;
	trans.m_values[Tz] = translationXYZ.z;

	Append(trans);
}

void Matrix4x4::AppendScaleUniform2D(float uniformScaleXY)
{
	Matrix4x4 scale;

	scale.m_values[Ix] = uniformScaleXY;
	scale.m_values[Jy] = uniformScaleXY;

	Append(scale);
}

void Matrix4x4::AppendScaleUniform3D(float uniformScaleXYZ)
{
	Matrix4x4 scale;

	scale.m_values[Ix] = uniformScaleXYZ;
	scale.m_values[Jy] = uniformScaleXYZ;
	scale.m_values[Kz] = uniformScaleXYZ;

	Append(scale);
}

void Matrix4x4::AppendScaleNonUniform2D(Vec2 const nonUniformScaleXY)
{
	Matrix4x4 scale;

	scale.m_values[Ix] = nonUniformScaleXY.x;
	scale.m_values[Jy] = nonUniformScaleXY.y;

	Append(scale);
}

void Matrix4x4::AppendScaleNonUniform3D(Vec3 const nonUniformScaleXYZ)
{
	Matrix4x4 scale;

	scale.m_values[Ix] = nonUniformScaleXYZ.x;
	scale.m_values[Jy] = nonUniformScaleXYZ.y;
	scale.m_values[Kz] = nonUniformScaleXYZ.z;

	Append(scale);
}

void Matrix4x4::Transpose()
{
	Matrix4x4 transposed;
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			transposed.m_values[row * 4 + col] = m_values[col * 4 + row];
		}
	}

	for (int i = 0; i < 16; ++i)
	{
		m_values[i] = transposed.m_values[i];
	}
}

void Matrix4x4::Orthonormalize_XFwd_YLeft_ZUp2()
{
	Vec4 iBasis = GetIBasis4D();
	iBasis.Normalize();

	Vec4 jBasis = GetJBasis4D();
	jBasis -= DotProduct4D(iBasis, jBasis) * iBasis;
	jBasis.Normalize();

	Vec4 kBasis = GetKBasis4D();
	kBasis -= DotProduct4D(iBasis, kBasis) * iBasis;
	kBasis -= DotProduct4D(jBasis, kBasis) * jBasis;
	kBasis.Normalize();

	Vec4 translation = GetTranslation4D();

	SetIJKT4D(iBasis, jBasis, kBasis, translation);
}

Matrix4x4 Matrix4x4::GetOrthonormalInverse()
{
	Matrix4x4 inv = *this;
	inv.SetTranslation3D(Vec3(0.f, 0.f, 0.f));
	inv.Transpose();

	Vec3 invTranslate(-m_values[Tx], -m_values[Ty], -m_values[Tz]);
	inv.AppendTranslation3D(invTranslate);

	return inv;
}
