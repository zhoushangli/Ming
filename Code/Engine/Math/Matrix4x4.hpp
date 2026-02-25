#pragma once

struct Vec2;
struct Vec3;
struct Vec4;

struct Matrix4x4
{
public:
    Matrix4x4();
    explicit Matrix4x4(float const* sixteenValuesBasisMajor);
    explicit Matrix4x4(
        float ix, float jx, float kx, float tx,
        float iy, float jy, float ky, float ty,
        float iz, float jz, float kz, float tz,
        float iw, float jw, float kw, float tw
    );
    explicit Matrix4x4(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translation2D);
    explicit Matrix4x4(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translation3D);
    explicit Matrix4x4(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& kBasis2D, Vec4 const& translation4D);
    explicit Matrix4x4(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D);

    static Matrix4x4 const MakeTranslation2D(Vec2 const translation);
    static Matrix4x4 const MakeTranslation3D(Vec3 const translationXYZ);
    static Matrix4x4 const MakeUniformScale2D(float uniformScaleXY);
    static Matrix4x4 const MakeUniformScale3D(float uniformScaleXYZ);
    static Matrix4x4 const MakeNonUniformScale2D(Vec2 const nonUniformScaleXY);
    static Matrix4x4 const MakeNonUniformScale3D(Vec3 const nonUniformScaleXYZ);
    static Matrix4x4 const MakeRotationDegrees2D(float rotationDegreesAboutZ);
    static Matrix4x4 const MakeRotationDegreesX(float rotationDegreesAboutX);
    static Matrix4x4 const MakeRotationDegreesY(float rotationDegreesAboutY);
    static Matrix4x4 const MakeRotationDegreesZ(float rotationDegreesAboutZ);

    static Matrix4x4 const MakeOrthoProjection(float left, float right, float bottom, float top, float zNear, float zFar);
    static Matrix4x4 const MakePerspectiveProjection(float fovDegrees, float aspectRatio, float zNear, float zFar);

    Vec2 const TransformVectorQuantity2D(Vec2 const vectorQuantityXY) const;        // assumes z=0, w=0
    Vec3 const TransformVectorQuantity3D(Vec3 const vectorQuantityXYZ) const;       // assumes w=0
    Vec2 const TransformPosition2D(Vec2 const positionXY) const;                    // assumes z=0, w=1
    Vec3 const TransformPosition3D(Vec3 const position3D) const;                    // assumes w=1
    Vec4 const TransformHomogeneous3D(Vec4 const homogeneous3D) const;              // w is provided

    float* GetAsFloatArray();               // non-const (mutable) version
    float const* GetAsFloatArray() const;   // const version, used only when Mat44 is const

    Vec2 const GetIBasis2D() const;
    Vec2 const GetJBasis2D() const;
    Vec2 const GetTranslation2D() const;

    Vec3 const GetIBasis3D() const;
    Vec3 const GetJBasis3D() const;
    Vec3 const GetKBasis3D() const;
    Vec3 const GetTranslation3D() const;

    Vec4 const GetIBasis4D() const;
    Vec4 const GetJBasis4D() const;
    Vec4 const GetKBasis4D() const;
    Vec4 const GetTranslation4D() const;

    void SetTranslation2D(Vec2 const translationXY);    
    void SetTranslation3D(Vec3 const translationXYZ);   
    void SetIJ2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D);
    void SetIJT2D(Vec2 const& iBasis2D, Vec2 const& jBasis2D, Vec2 const& translationXY);
    void SetIJK3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D);
    void SetIJKT3D(Vec3 const& iBasis3D, Vec3 const& jBasis3D, Vec3 const& kBasis3D, Vec3 const& translationXYZ);
    void SetIJKT4D(Vec4 const& iBasis4D, Vec4 const& jBasis4D, Vec4 const& kBasis4D, Vec4 const& translation4D); 

    void Append(Matrix4x4 const appendThis);            
    void AppendRotationX(float degreesAboutX);
    void AppendRotationY(float degreesAboutY);          
    void AppendRotationZ(float degreesAboutZ);          
    void AppendTranslation2D(Vec2 const translationXY); 
    void AppendTranslation3D(Vec3 const translationXYZ);
    void AppendScaleUniform2D(float uniformScaleXY);    
    void AppendScaleUniform3D(float uniformScaleXYZ);
    void AppendScaleNonUniform2D(Vec2 const nonUniformScaleXY);    
    void AppendScaleNonUniform3D(Vec3 const nonUniformScaleXYZ);

    void Transpose();
    void Orthonormalize_XFwd_YLeft_ZUp2();
    Matrix4x4 GetOrthonormalInverse();

    static Matrix4x4 const ZERO;
    static Matrix4x4 const IDENTITY;

public:
    enum { Ix, Iy, Iz, Iw, Jx, Jy, Jz, Jw, Kx, Ky, Kz, Kw, Tx, Ty, Tz, Tw }; // index nicknames

    float m_values[16]; 
};
