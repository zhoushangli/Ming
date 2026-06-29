#include "MingEngine/Engine/Script/RegisterBuildinType.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Math/Capsule3.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/OBB2.hpp"
#include "MingEngine/Core/Math/Vec2.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Math/Vec4.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Variant.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Script/ScriptTypeUtils.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace
{
void ConstructVec2(void* memory) { new (memory) Vec2(); }

void ConstructVec2WithXY(float x, float y, void* memory) { new (memory) Vec2(x, y); }

void CopyConstructVec2(Vec2 const& other, void* memory) { new (memory) Vec2(other); }

void DestructVec2(void* memory) { static_cast<Vec2*>(memory)->~Vec2(); }

Vec2& AssignVec2(Vec2 const& other, Vec2* self)
{
	*self = other;
	return *self;
}

Vec2& AddAssignVec2(Vec2 const& other, Vec2* self)
{
	*self += other;
	return *self;
}

Vec2& SubtractAssignVec2(Vec2 const& other, Vec2* self)
{
	*self -= other;
	return *self;
}

Vec2& MultiplyAssignVec2(float scale, Vec2* self)
{
	*self *= scale;
	return *self;
}

Vec2& DivideAssignVec2(float divisor, Vec2* self)
{
	*self /= divisor;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructVec3(void* memory) { new (memory) Vec3(); }

void ConstructVec3WithXYZ(float x, float y, float z, void* memory) { new (memory) Vec3(x, y, z); }

void CopyConstructVec3(Vec3 const& other, void* memory) { new (memory) Vec3(other); }

void DestructVec3(void* memory) { static_cast<Vec3*>(memory)->~Vec3(); }

Vec3& AssignVec3(Vec3 const& other, Vec3* self)
{
	*self = other;
	return *self;
}

Vec3& AddAssignVec3(Vec3 const& other, Vec3* self)
{
	*self += other;
	return *self;
}

Vec3& SubtractAssignVec3(Vec3 const& other, Vec3* self)
{
	*self -= other;
	return *self;
}

Vec3& MultiplyAssignVec3(float scale, Vec3* self)
{
	*self *= scale;
	return *self;
}

Vec3& DivideAssignVec3(float divisor, Vec3* self)
{
	*self /= divisor;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructVec4(void* memory) { new (memory) Vec4(); }

void ConstructVec4WithXYZW(float x, float y, float z, float w, void* memory) { new (memory) Vec4(x, y, z, w); }

void CopyConstructVec4(Vec4 const& other, void* memory) { new (memory) Vec4(other); }

void DestructVec4(void* memory) { static_cast<Vec4*>(memory)->~Vec4(); }

Vec4& AssignVec4(Vec4 const& other, Vec4* self)
{
	*self = other;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructAABB2(void* memory) { new (memory) AABB2(); }

void ConstructAABB2WithMinsMaxs(Vec2 const& mins, Vec2 const& maxs, void* memory)
{
	new (memory) AABB2(mins, maxs);
}

void ConstructAABB2WithFloats(float minX, float minY, float maxX, float maxY, void* memory)
{
	new (memory) AABB2(minX, minY, maxX, maxY);
}

void CopyConstructAABB2(AABB2 const& other, void* memory) { new (memory) AABB2(other); }

void DestructAABB2(void* memory) { static_cast<AABB2*>(memory)->~AABB2(); }

AABB2& AssignAABB2(AABB2 const& other, AABB2* self)
{
	*self = other;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructOBB2(void* memory) { new (memory) OBB2(); }

void ConstructOBB2WithBasis(Vec2 const& center, Vec2 const& iBasisNormal, Vec2 const& halfDimensions, void* memory)
{
	new (memory) OBB2(center, iBasisNormal, halfDimensions);
}

void ConstructOBB2WithOrientation(Vec2 const& center, Vec2 const& halfDimensions, float orientationDegree, void* memory)
{
	new (memory) OBB2(center, halfDimensions, orientationDegree);
}

void CopyConstructOBB2(OBB2 const& other, void* memory) { new (memory) OBB2(other); }

void DestructOBB2(void* memory) { static_cast<OBB2*>(memory)->~OBB2(); }

OBB2& AssignOBB2(OBB2 const& other, OBB2* self)
{
	*self = other;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructCapsule3(void* memory) { new (memory) Capsule3(); }

void ConstructCapsule3WithStartEndRadius(Vec3 const& start, Vec3 const& end, float radius, void* memory)
{
	new (memory) Capsule3(start, end, radius);
}

void CopyConstructCapsule3(Capsule3 const& other, void* memory) { new (memory) Capsule3(other); }

void DestructCapsule3(void* memory) { static_cast<Capsule3*>(memory)->~Capsule3(); }

Capsule3& AssignCapsule3(Capsule3 const& other, Capsule3* self)
{
	*self = other;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructEulerAngles(void* memory) { new (memory) EulerAngles(); }

void ConstructEulerAnglesWithXYZ(float x, float y, float z, void* memory) { new (memory) EulerAngles(x, y, z); }

void CopyConstructEulerAngles(EulerAngles const& other, void* memory) { new (memory) EulerAngles(other); }

void DestructEulerAngles(void* memory) { static_cast<EulerAngles*>(memory)->~EulerAngles(); }

EulerAngles& AssignEulerAngles(EulerAngles const& other, EulerAngles* self)
{
	*self = other;
	return *self;
}

EulerAngles& AddAssignEulerAngles(EulerAngles const& other, EulerAngles* self)
{
	*self += other;
	return *self;
}

EulerAngles& MultiplyAssignEulerAngles(float scale, EulerAngles* self)
{
	*self *= scale;
	return *self;
}

EulerAngles MakeEulerAnglesFromForward(Vec3 const& forward, EulerAngles const*)
{
	return EulerAngles::MakeFromForward(forward);
}

// ------------------------------------------------------------------------------

void ConstructMatrix4x4(void* memory) { new (memory) Matrix4x4(); }

void CopyConstructMatrix4x4(Matrix4x4 const& other, void* memory) { new (memory) Matrix4x4(other); }

void DestructMatrix4x4(void* memory) { static_cast<Matrix4x4*>(memory)->~Matrix4x4(); }

Matrix4x4& AssignMatrix4x4(Matrix4x4 const& other, Matrix4x4* self)
{
	*self = other;
	return *self;
}

// ------------------------------------------------------------------------------

void ConstructVariant(void* memory) { new (memory) Variant(); }

void CopyConstructVariant(Variant const& other, void* memory) { new (memory) Variant(other); }

void DestructVariant(void* memory) { static_cast<Variant*>(memory)->~Variant(); }

Variant& AssignVariant(Variant const& other, Variant* self)
{
	*self = other;
	return *self;
}

} // namespace

void RegisterVec2(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("Vec2", sizeof(Vec2), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: Vec2");

	result =
		engine->RegisterObjectBehaviour("Vec2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec2");

	result = engine->RegisterObjectBehaviour(
		"Vec2",
		asBEHAVE_CONSTRUCT,
		"void f(float, float)",
		asFUNCTION(ConstructVec2WithXY),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec2");

	result = engine->RegisterObjectBehaviour(
		"Vec2",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec2 &in)",
		asFUNCTION(CopyConstructVec2),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec2");

	result =
		engine->RegisterObjectBehaviour("Vec2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: Vec2");

	result = engine->RegisterObjectMethod("Vec2", "Vec2 &opAssign(const Vec2 &in)", asFUNCTION(AssignVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: Vec2");

	result = engine->RegisterObjectMethod(
		"Vec2",
		"bool opEquals(const Vec2 &in) const",
		asMETHODPR(Vec2, operator==, (Vec2 const&) const, bool),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register equality operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod(
		"Vec2",
		"Vec2 opAdd(const Vec2 &in) const",
		asMETHODPR(Vec2, operator+, (Vec2 const&) const, Vec2 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register add operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod(
		"Vec2",
		"Vec2 opSub(const Vec2 &in) const",
		asMETHODPR(Vec2, operator-, (Vec2 const&) const, Vec2 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register subtract operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod(
		"Vec2",
		"Vec2 opNeg() const",
		asMETHODPR(Vec2, operator-, () const, Vec2 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register negation operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod(
		"Vec2",
		"Vec2 opMul(float) const",
		asMETHODPR(Vec2, operator*, (float) const, Vec2 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register scale operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod(
		"Vec2",
		"Vec2 opMul(const Vec2 &in) const",
		asMETHODPR(Vec2, operator*, (Vec2 const&) const, Vec2 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register multiply operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod(
		"Vec2",
		"Vec2 opDiv(float) const",
		asMETHODPR(Vec2, operator/, (float) const, Vec2 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register divide operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "Vec2 &opAddAssign(const Vec2 &in)", asFUNCTION(AddAssignVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register add-assign operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "Vec2 &opSubAssign(const Vec2 &in)", asFUNCTION(SubtractAssignVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register subtract-assign operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "Vec2 &opMulAssign(float)", asFUNCTION(MultiplyAssignVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register multiply-assign operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "Vec2 &opDivAssign(float)", asFUNCTION(DivideAssignVec2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register divide-assign operator for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "float GetLength() const", asMETHOD(Vec2, GetLength), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetLength for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "float GetLengthSquared() const", asMETHOD(Vec2, GetLengthSquared), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetLengthSquared for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "Vec2 GetClamped(float) const", asMETHOD(Vec2, GetClamped), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetClamped for built-in type: Vec2");
	result = engine->RegisterObjectMethod("Vec2", "Vec2 GetNormalized() const", asMETHOD(Vec2, GetNormalized), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetNormalized for built-in type: Vec2");

	result = engine->RegisterObjectProperty("Vec2", "float x", asOFFSET(Vec2, x));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'x' for built-in type: Vec2");
	result = engine->RegisterObjectProperty("Vec2", "float y", asOFFSET(Vec2, y));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'y' for built-in type: Vec2");
}

void RegisterVec3(asIScriptEngine* engine)
{
	int result = 0;

	// Register Vec3
	result = engine->RegisterObjectType("Vec3", sizeof(Vec3), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: Vec3");

	result = engine->RegisterObjectBehaviour(
		"Vec3",
		asBEHAVE_CONSTRUCT,
		"void f()",
		asFUNCTION(ConstructVec3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec3");

	result = engine->RegisterObjectBehaviour(
		"Vec3",
		asBEHAVE_CONSTRUCT,
		"void f(float, float, float)",
		asFUNCTION(ConstructVec3WithXYZ),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec3");

	result = engine->RegisterObjectBehaviour(
		"Vec3",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec3 &in)",
		asFUNCTION(CopyConstructVec3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec3");

	result = engine->RegisterObjectBehaviour(
		"Vec3",
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(DestructVec3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: Vec3");

	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 &opAssign(const Vec3 &in)",
		asFUNCTION(AssignVec3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: Vec3");

	result = engine->RegisterObjectProperty("Vec3", "float x", asOFFSET(Vec3, x));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'x' for built-in type: Vec3");
	result = engine->RegisterObjectProperty("Vec3", "float y", asOFFSET(Vec3, y));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'y' for built-in type: Vec3");
	result = engine->RegisterObjectProperty("Vec3", "float z", asOFFSET(Vec3, z));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'z' for built-in type: Vec3");

	result = engine->RegisterObjectMethod(
		"Vec3",
		"bool opEquals(const Vec3 &in) const",
		asMETHODPR(Vec3, operator==, (Vec3 const&) const, bool),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register equality operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 opAdd(const Vec3 &in) const",
		asMETHODPR(Vec3, operator+, (Vec3 const&) const, Vec3 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register add operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 opSub(const Vec3 &in) const",
		asMETHODPR(Vec3, operator-, (Vec3 const&) const, Vec3 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register subtract operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 opNeg() const",
		asMETHODPR(Vec3, operator-, () const, Vec3 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register negation operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 opMul(float) const",
		asMETHODPR(Vec3, operator*, (float) const, Vec3 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register scale operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 opMul(const Vec3 &in) const",
		asMETHODPR(Vec3, operator*, (Vec3 const&) const, Vec3 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register multiply operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod(
		"Vec3",
		"Vec3 opDiv(float) const",
		asMETHODPR(Vec3, operator/, (float) const, Vec3 const),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register divide operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "Vec3 &opAddAssign(const Vec3 &in)", asFUNCTION(AddAssignVec3), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register add-assign operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "Vec3 &opSubAssign(const Vec3 &in)", asFUNCTION(SubtractAssignVec3), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register subtract-assign operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "Vec3 &opMulAssign(float)", asFUNCTION(MultiplyAssignVec3), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register multiply-assign operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "Vec3 &opDivAssign(float)", asFUNCTION(DivideAssignVec3), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register divide-assign operator for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "float GetLength() const", asMETHOD(Vec3, GetLength), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetLength for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "float GetLengthSquared() const", asMETHOD(Vec3, GetLengthSquared), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetLengthSquared for built-in type: Vec3");
	result = engine->RegisterObjectMethod("Vec3", "Vec3 GetNormalized() const", asMETHOD(Vec3, GetNormalized), asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetNormalized for built-in type: Vec3");
}

void RegisterVec4(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("Vec4", sizeof(Vec4), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: Vec4");

	result =
		engine->RegisterObjectBehaviour("Vec4", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructVec4), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec4");

	result = engine->RegisterObjectBehaviour(
		"Vec4",
		asBEHAVE_CONSTRUCT,
		"void f(float, float, float, float)",
		asFUNCTION(ConstructVec4WithXYZW),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec4");

	result = engine->RegisterObjectBehaviour(
		"Vec4",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec4 &in)",
		asFUNCTION(CopyConstructVec4),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Vec4");

	result =
		engine->RegisterObjectBehaviour("Vec4", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructVec4), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: Vec4");

	result = engine->RegisterObjectMethod("Vec4", "Vec4 &opAssign(const Vec4 &in)", asFUNCTION(AssignVec4), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: Vec4");

	result = engine->RegisterObjectProperty("Vec4", "float x", asOFFSET(Vec4, x));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'x' for built-in type: Vec4");
	result = engine->RegisterObjectProperty("Vec4", "float y", asOFFSET(Vec4, y));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'y' for built-in type: Vec4");
	result = engine->RegisterObjectProperty("Vec4", "float z", asOFFSET(Vec4, z));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'z' for built-in type: Vec4");
	result = engine->RegisterObjectProperty("Vec4", "float w", asOFFSET(Vec4, w));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'w' for built-in type: Vec4");
}

void RegisterAABB2(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("AABB2", sizeof(AABB2), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: AABB2");

	result =
		engine->RegisterObjectBehaviour("AABB2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructAABB2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: AABB2");

	result = engine->RegisterObjectBehaviour(
		"AABB2",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec2 &in, const Vec2 &in)",
		asFUNCTION(ConstructAABB2WithMinsMaxs),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: AABB2");

	result = engine->RegisterObjectBehaviour(
		"AABB2",
		asBEHAVE_CONSTRUCT,
		"void f(float, float, float, float)",
		asFUNCTION(ConstructAABB2WithFloats),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: AABB2");

	result = engine->RegisterObjectBehaviour(
		"AABB2",
		asBEHAVE_CONSTRUCT,
		"void f(const AABB2 &in)",
		asFUNCTION(CopyConstructAABB2),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: AABB2");

	result = engine->RegisterObjectBehaviour(
		"AABB2",
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(DestructAABB2),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: AABB2");

	result = engine->RegisterObjectMethod("AABB2", "AABB2 &opAssign(const AABB2 &in)", asFUNCTION(AssignAABB2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: AABB2");

	result = engine->RegisterObjectProperty("AABB2", "Vec2 mins", asOFFSET(AABB2, m_mins));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'mins' for built-in type: AABB2");
	result = engine->RegisterObjectProperty("AABB2", "Vec2 maxs", asOFFSET(AABB2, m_maxs));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'maxs' for built-in type: AABB2");
}

void RegisterOBB2(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("OBB2", sizeof(OBB2), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: OBB2");

	result =
		engine->RegisterObjectBehaviour("OBB2", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructOBB2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: OBB2");

	result = engine->RegisterObjectBehaviour(
		"OBB2",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec2 &in, const Vec2 &in, const Vec2 &in)",
		asFUNCTION(ConstructOBB2WithBasis),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: OBB2");

	result = engine->RegisterObjectBehaviour(
		"OBB2",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec2 &in, const Vec2 &in, float)",
		asFUNCTION(ConstructOBB2WithOrientation),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: OBB2");

	result = engine->RegisterObjectBehaviour(
		"OBB2",
		asBEHAVE_CONSTRUCT,
		"void f(const OBB2 &in)",
		asFUNCTION(CopyConstructOBB2),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: OBB2");

	result =
		engine->RegisterObjectBehaviour("OBB2", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructOBB2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: OBB2");

	result = engine->RegisterObjectMethod("OBB2", "OBB2 &opAssign(const OBB2 &in)", asFUNCTION(AssignOBB2), asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: OBB2");

	result = engine->RegisterObjectProperty("OBB2", "Vec2 center", asOFFSET(OBB2, m_center));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'center' for built-in type: OBB2");
	result = engine->RegisterObjectProperty("OBB2", "Vec2 iBasisNormal", asOFFSET(OBB2, m_iBasisNormal));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'iBasisNormal' for built-in type: OBB2");
	result = engine->RegisterObjectProperty("OBB2", "Vec2 halfDimensions", asOFFSET(OBB2, m_halfDimensions));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'halfDimensions' for built-in type: OBB2");
}

void RegisterCapsule3(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("Capsule3", sizeof(Capsule3), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: Capsule3");

	result = engine->RegisterObjectBehaviour(
		"Capsule3",
		asBEHAVE_CONSTRUCT,
		"void f()",
		asFUNCTION(ConstructCapsule3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Capsule3");

	result = engine->RegisterObjectBehaviour(
		"Capsule3",
		asBEHAVE_CONSTRUCT,
		"void f(const Vec3 &in, const Vec3 &in, float)",
		asFUNCTION(ConstructCapsule3WithStartEndRadius),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Capsule3");

	result = engine->RegisterObjectBehaviour(
		"Capsule3",
		asBEHAVE_CONSTRUCT,
		"void f(const Capsule3 &in)",
		asFUNCTION(CopyConstructCapsule3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Capsule3");

	result = engine->RegisterObjectBehaviour(
		"Capsule3",
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(DestructCapsule3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: Capsule3");

	result = engine->RegisterObjectMethod(
		"Capsule3",
		"Capsule3 &opAssign(const Capsule3 &in)",
		asFUNCTION(AssignCapsule3),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: Capsule3");

	result = engine->RegisterObjectProperty("Capsule3", "Vec3 start", asOFFSET(Capsule3, m_start));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'start' for built-in type: Capsule3");
	result = engine->RegisterObjectProperty("Capsule3", "Vec3 end", asOFFSET(Capsule3, m_end));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'end' for built-in type: Capsule3");
	result = engine->RegisterObjectProperty("Capsule3", "float radius", asOFFSET(Capsule3, m_radius));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'radius' for built-in type: Capsule3");
}

void RegisterEulerAngles(asIScriptEngine* engine)
{
	int result = 0;

	// Register EulerAngles
	result = engine->RegisterObjectType("EulerAngles", sizeof(EulerAngles), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: EulerAngles");

	result = engine->RegisterObjectBehaviour(
		"EulerAngles",
		asBEHAVE_CONSTRUCT,
		"void f()",
		asFUNCTION(ConstructEulerAngles),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: EulerAngles");

	result = engine->RegisterObjectBehaviour(
		"EulerAngles",
		asBEHAVE_CONSTRUCT,
		"void f(float, float, float)",
		asFUNCTION(ConstructEulerAnglesWithXYZ),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: EulerAngles");

	result = engine->RegisterObjectBehaviour(
		"EulerAngles",
		asBEHAVE_CONSTRUCT,
		"void f(const EulerAngles &in)",
		asFUNCTION(CopyConstructEulerAngles),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: EulerAngles");

	result = engine->RegisterObjectBehaviour(
		"EulerAngles",
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(DestructEulerAngles),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: EulerAngles");

	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles &opAssign(const EulerAngles &in)",
		asFUNCTION(AssignEulerAngles),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: EulerAngles");

	result = engine->RegisterObjectProperty("EulerAngles", "float yawDegrees", asOFFSET(EulerAngles, m_yawDegrees));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'yawDegrees' for built-in type: EulerAngles");
	result = engine->RegisterObjectProperty("EulerAngles", "float pitchDegrees", asOFFSET(EulerAngles, m_pitchDegrees));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'pitchDegrees' for built-in type: EulerAngles");
	result = engine->RegisterObjectProperty("EulerAngles", "float rollDegrees", asOFFSET(EulerAngles, m_rollDegrees));
	GUARANTEE_OR_DIE(result >= 0, "Failed to register property 'rollDegrees' for built-in type: EulerAngles");

	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"bool opEquals(const EulerAngles &in) const",
		asMETHODPR(EulerAngles, operator==, (EulerAngles const&) const, bool),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register equality operator for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles opAdd(const EulerAngles &in) const",
		asMETHODPR(EulerAngles, operator+, (EulerAngles const&) const, EulerAngles),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register add operator for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles opSub(const EulerAngles &in) const",
		asMETHODPR(EulerAngles, operator-, (EulerAngles const&) const, EulerAngles),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register subtract operator for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles opMul(float) const",
		asMETHODPR(EulerAngles, operator*, (float) const, EulerAngles),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register scale operator for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles &opAddAssign(const EulerAngles &in)",
		asFUNCTION(AddAssignEulerAngles),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register add-assign operator for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles &opMulAssign(float)",
		asFUNCTION(MultiplyAssignEulerAngles),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register multiply-assign operator for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"Vec3 GetForwardDir_IFwd_JLeft_KUp() const",
		asMETHOD(EulerAngles, GetForwardDir_IFwd_JLeft_KUp),
		asCALL_THISCALL);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register GetForwardDir_IFwd_JLeft_KUp for built-in type: EulerAngles");
	result = engine->RegisterObjectMethod(
		"EulerAngles",
		"EulerAngles MakeFromForward(const Vec3 &in) const",
		asFUNCTION(MakeEulerAnglesFromForward),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register MakeFromForward for built-in type: EulerAngles");
}

void RegisterMatrix4x4(asIScriptEngine* engine)
{
	int result = 0;

	// Register Matrix4x4
	result = engine->RegisterObjectType("Matrix4x4", sizeof(Matrix4x4), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: Matrix4x4");

	result = engine->RegisterObjectBehaviour(
		"Matrix4x4",
		asBEHAVE_CONSTRUCT,
		"void f()",
		asFUNCTION(ConstructMatrix4x4),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Matrix4x4");

	result = engine->RegisterObjectBehaviour(
		"Matrix4x4",
		asBEHAVE_CONSTRUCT,
		"void f(const Matrix4x4 &in)",
		asFUNCTION(CopyConstructMatrix4x4),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register constructor for built-in type: Matrix4x4");

	result = engine->RegisterObjectBehaviour(
		"Matrix4x4",
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(DestructMatrix4x4),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: Matrix4x4");

	result = engine->RegisterObjectMethod(
		"Matrix4x4",
		"Matrix4x4 &opAssign(const Matrix4x4 &in)",
		asFUNCTION(AssignMatrix4x4),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: Matrix4x4");
}

void RegisterVariant(asIScriptEngine* engine)
{
	int result = 0;

	result = engine->RegisterObjectType("Variant", sizeof(Variant), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register built-in type: Variant");

	result = engine->RegisterObjectBehaviour(
		"Variant",
		asBEHAVE_CONSTRUCT,
		"void f()",
		asFUNCTION(ConstructVariant),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register default constructor for built-in type: Variant");

	result = engine->RegisterObjectBehaviour(
		"Variant",
		asBEHAVE_CONSTRUCT,
		"void f(const Variant &in)",
		asFUNCTION(CopyConstructVariant),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register copy constructor for built-in type: Variant");

	result = engine->RegisterObjectBehaviour(
		"Variant",
		asBEHAVE_DESTRUCT,
		"void f()",
		asFUNCTION(DestructVariant),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register destructor for built-in type: Variant");

	result = engine->RegisterObjectMethod(
		"Variant",
		"Variant &opAssign(const Variant &in)",
		asFUNCTION(AssignVariant),
		asCALL_CDECL_OBJLAST);
	GUARANTEE_OR_DIE(result >= 0, "Failed to register assignment operator for built-in type: Variant");
}
