#pragma once

class asIScriptEngine;

void RegisterVec3(asIScriptEngine* engine);
void RegisterEulerAngles(asIScriptEngine* engine);
void RegisterMatrix4x4(asIScriptEngine* engine);

// The only script-visible class is Object, we call it NativeObject in the script
// and we we want to call certain function, we use something like this:
// __Call_Void(NativeObject@ nativePtr, const string &in className, const string &in methodName, ...)
void RegisterNativeObjectType(asIScriptEngine* engine);

// Register bridge functions for all registered classes and methods in the ClassDatabase
// For functions in the same layout, like
// Node3D::SetScale(const Vec3&) and Node3D::SetPosition(const Vec3&)
// we only register one bridge function, like
// void __Call_Void_Vec3(NativeObject@ nativePtr, const string &in className, const string &in methodName, const Vec3 &in arg0)
void RegisterBridgeFunctions(asIScriptEngine* engine);
void GenerateBuiltinScript();
