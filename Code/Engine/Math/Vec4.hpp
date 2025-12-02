#pragma once

struct Vec2;
struct Vec3;
struct Vec4;

// 4x4 Homogeneous 2D/3D transformation matrix, stored basis-major in memory (Ix,Iy,Iz,Iw, Jx,Jy,Jz,Jw, Kx,Ky,Kz,Kw, Tx,Ty,Tz,Tw…).

// Note: we specifically do NOT provide an operator* overload, since doing so would require a
// decision to commit the Mat44 to only work consistently with either column-major or row-major style
// notation. They demand two different ways of writing operator*, and in order to implement 
// an operator*, we are forced to make a notational commitment. This is certainly ambiguous to the
// students, and at the very least, potentially confusing. Instead, we prefer to use method names, 
// such as “Append”, which are more neutral (e.g. multiply a new matrix *on the right in column-
// notation / on the left in row-notation* )

struct Mat44
{
    enum { Ix, Iy, Iz, Iw, Jx, Jy, Jz, Jw, Kx, Ky, Kz, Kw, Tx, Ty, Tz, Tw }; // index nicknames, [0] thru [15]
    float m_values[16]; // stored in “basis major” order (Ix,Iy,Iz,Iw, Jx… ) - translation in [12,13,14]

    Mat44(); // Default creates the IDENTITY matrix!
    explicit Mat44(Vec2 const& basisI2D, Vec2 const& basisJ2D, Vec2 const& translation2D);
    explicit Mat44(Vec3 const& basisI3D, Vec3 const& basisJ3D, Vec3 const& basisK3D, Vec3 const& translation3D);
    explicit Mat44(Vec2 const& basisI2D, Vec2 const& basisJ2D, Vec2 const& basisK2D, Vec4 const& translation4D);
    explicit Mat44(float const sixteenValuesBasisMajor);

    static Mat44 const MakeTranslation2D(Vec2 const translation);
    static Mat44 const MakeTranslation3D(Vec3 const translationXYZ);
    static Mat44 const MakeUniformScale2D(float uniformScaleXY);
    static Mat44 const MakeUniformScale3D(float uniformScaleXYZ);
    static Mat44 const MakeNonUniformScale2D(Vec2 const nonUniformScaleXY);
    static Mat44 const MakeNonUniformScale3D(Vec3 const nonUniformScaleXYZ);
    static Mat44 const MakeRotationDegrees2D(float rotationDegreesAboutZ);
    static Mat44 const MakeRotationDegreesX(float rotationDegreesAboutX);
    static Mat44 const MakeRotationDegreesY(float rotationDegreesAboutY);
    static Mat44 const MakeRotationDegreesZ(float rotationDegreesAboutZ);

    Vec2 const TransformVectorQuantity2D(Vec2 const vectorQuantityXY) const; // assumes z=0, w=0
    Vec3 const TransformVectorQuantity3D(Vec3 const vectorQuantityXYZ) const; // assumes w=0
    Vec2 const TransformPosition2D(Vec2 const positionXY) const; // assumes z=0, w=1
    Vec3 const TransformPosition3D(Vec3 const position3D) const; // assumes w=1
    Vec4 const TransformHomogeneousPoint3D(Vec4 const homogeneous3DPoint) const; // w is provided

    float const* GetAsFloatArray() const; // non-const (mutable) version
    float const* GetAsFloatArray() const; // const version, used only when Mat44 is const

    Vec2 const GetBasisI2D() const;
    Vec2 const GetBasisJ2D() const;
    Vec2 const GetTranslation2D() const;

    Vec3 const GetBasisI3D() const;
    Vec3 const GetBasisJ3D() const;
    Vec3 const GetBasisK3D() const;
    Vec3 const GetTranslation3D() const;

    Vec4 const GetBasisI4D() const;
    Vec4 const GetBasisJ4D() const;
    Vec4 const GetBasisK4D() const;
    Vec4 const GetTranslation4D() const;

    void SetTranslation2D(Vec2 const translationXY); // Sets translationZ = 0, translationW = 1
    void SetTranslation3D(Vec3 const translationXYZ); // Sets translationW = 1
    void SetIJ2D(Vec2 const& basisI2D, Vec2 const& basisJ2D); // Sets z=0 for I & J; does not modify K or T
    void SetIJK2D(Vec2 const& basisI2D, Vec2 const& basisJ2D, Vec2 const& basisK2D, Vec2 const& translationXY); // w=1; does not modify K
    void SetIJK3D(Vec3 const& basisI3D, Vec3 const& basisJ3D, Vec3 const& basisK3D); // Sets w=0 for IJK; does not modify T
    void SetIJK3D(Vec3 const& basisI3D, Vec3 const& basisJ3D, Vec3 const& basisK3D, Vec3 const& translationXYZ); // Sets w for IJK, w=1 for T
    void SetIJK4D(Vec4 const& basisI4D, Vec4 const& basisJ4D, Vec4 const& basisK4D, Vec4 const& translation4D); // All 16 values provided

    void Append(Mat44 const appendThis); // multiply on right in column notation / on left in row notation
    void AppendRotation(float degreesAboutX); // same as appending (in column notation) a x-rotation matrix
    void AppendRotationY(float degreesAboutY); // same as appending (in column notation) a y-rotation matrix
    void AppendRotationZ(float degreesAboutZ); // same as appending (in column notation) a z-rotation matrix
    void AppendTranslation2D(Vec2 const translationXY); // same as appending (in column notation) a translation matrix
    void AppendTranslation3D(Vec3 const translationXYZ); // same as appending (in column notation) a translation matrix
    void AppendScaleUniform2D(float uniformScaleXY); // translation should remain unaffected
    void AppendScaleUniform3D(float uniformScaleXYZ);
    void AppendScaleNonUniform2D(Vec2 const nonUniformScaleXY); // X and Y bases should remain orthogonal
    void AppendScaleNonUniform3D(Vec3 const nonUniformScaleXYZ); // translation should remain unaffected
};
