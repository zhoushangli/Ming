#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <string>

constexpr char const* kNativeObjectTypeName     = "NativeObject";
constexpr char const* kNativeObjectPropertyName = "nativePtr";

// Object: script-visible class methods backed by a NativeObject@ instance.
// Example: class Node3D { void SetPosition(...) { __Call_...(nativePtr, "Node3D", "SetPosition", ...); } }
//
// GlobalObject: script-visible namespace functions backed by a registered global object.
// Example: namespace InputSystem { bool IsKeyPressed(...) { return __Call_GlobalObject_...("InputSystem", "IsKeyPressed", ...); } }
//
// Global: script-visible namespace functions backed by native free functions.
// Example: namespace Debug { void Log(...) { __Call_Global_Log("Debug", "Log", ...); } }
enum class ScriptCallableKind
{
	Object,
	GlobalObject,
	Global,
};

std::string GetScriptTypeName(Variant::Type type);
std::string GetBridgeTypeName(Variant::Type type);
bool        IsScriptRefType(Variant::Type type);
std::string BuildScriptArgumentDeclaration(Variant::Type type);
std::string BuildBridgeFunctionName(MethodInfo const& methodInfo, ScriptCallableKind kind);
std::string BuildBridgeFunctionDeclaration(MethodInfo const& methodInfo, ScriptCallableKind kind);