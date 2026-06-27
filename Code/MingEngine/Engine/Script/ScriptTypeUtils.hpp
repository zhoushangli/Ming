#pragma once

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/Variant.hpp"

#include <string>

constexpr char const* kNativeObjectTypeName     = "NativeObject";
constexpr char const* kNativeObjectPropertyName = "nativePtr";

std::string GetScriptTypeName(Variant::Type type);
std::string GetBridgeTypeName(Variant::Type type);
bool        IsScriptRefType(Variant::Type type);
std::string BuildScriptArgumentDeclaration(Variant::Type type);
std::string BuildBridgeFunctionName(MethodInfo const& methodInfo);
std::string BuildBridgeFunctionDeclaration(MethodInfo const& methodInfo);
