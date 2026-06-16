#include "MingEngine/Engine/Script/RegisterBuildinType.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

#include <new>

namespace
{
void ConstructVec3(void* memory) { new (memory) Vec3(); }

void ConstructVec3WithXYZ(float x, float y, float z, void* memory) { new (memory) Vec3(x, y, z); }

void CopyConstructVec3(Vec3 const& other, void* memory) { new (memory) Vec3(other); }

void DestructVec3(void* memory) { static_cast<Vec3*>(memory)->~Vec3(); }

Vec3& AssignVec3(Vec3 const& other, Vec3* self)
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

// ------------------------------------------------------------------------------

void ConstructMatrix4x4(void* memory) { new (memory) Matrix4x4(); }

void CopyConstructMatrix4x4(Matrix4x4 const& other, void* memory) { new (memory) Matrix4x4(other); }

void DestructMatrix4x4(void* memory) { static_cast<Matrix4x4*>(memory)->~Matrix4x4(); }

Matrix4x4& AssignMatrix4x4(Matrix4x4 const& other, Matrix4x4* self)
{
	*self = other;
	return *self;
}

} // namespace

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