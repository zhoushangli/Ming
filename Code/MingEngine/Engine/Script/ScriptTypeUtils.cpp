#include "MingEngine/Engine/Script/ScriptTypeUtils.hpp"

#include <utility>
#include <vector>

namespace
{
// Bridge function templates.
// These declarations are registered directly with AngelScript through RegisterGlobalFunction.
// They must match the wrapper calls emitted by ScriptGenerator.cpp.

// Object bridge name example:
// __Call_Void_Vec3
constexpr char const* kObjectBridgeNameTemplate =
	R"AS(
__Call_${signature}
)AS";

// GlobalObject bridge name example:
// __Call_GlobalObject_Bool_Int
constexpr char const* kGlobalObjectBridgeNameTemplate =
	R"AS(
__Call_GlobalObject_${signature}
)AS";

// Global keeps the existing method-name bridge shape.
// Example:
// __Call_Global_Log
constexpr char const* kGlobalBridgeNameTemplate =
	R"AS(
__Call_Global_${methodName}
)AS";

// Object bridge declaration example:
// void __Call_Void_Vec3(NativeObject@ nativePtr, const string &in className, const string &in methodName, const Vec3
// &in arg0)
constexpr char const* kObjectBridgeDeclarationTemplate =
	R"AS(
${returnType} ${functionName}(NativeObject@ nativePtr, const string &in className, const string &in methodName${arguments})
)AS";

// GlobalObject bridge declaration example:
// bool __Call_GlobalObject_Bool_Int(const string &in className, const string &in methodName, int arg0)
constexpr char const* kGlobalObjectBridgeDeclarationTemplate =
	R"AS(
${returnType} ${functionName}(const string &in className, const string &in methodName${arguments})
)AS";

// Global bridge declaration example:
// void __Call_Global_Log(const string &in namespaceName, const string &in methodName, const string &in arg0)
constexpr char const* kGlobalBridgeDeclarationTemplate =
	R"AS(
${returnType} ${functionName}(const string &in namespaceName, const string &in methodName${arguments})
)AS";

// Bridge declarations are registered as single-line AngelScript declarations, so trim both edges.
std::string TrimTemplateText(char const* templateText)
{
	std::string text = templateText;
	if (!text.empty() && text.front() == '\n')
	{
		text.erase(text.begin());
	}
	if (!text.empty() && text.back() == '\n')
	{
		text.pop_back();
	}
	return text;
}

// Replaces simple `${name}` placeholders in a bridge template.
std::string ExpandTemplate(char const* templateText, std::vector<std::pair<std::string, std::string>> const& values)
{
	std::string result = TrimTemplateText(templateText);
	for (std::pair<std::string, std::string> const& value : values)
	{
		std::string const placeholder = "${" + value.first + "}";
		size_t            position    = 0;
		while ((position = result.find(placeholder, position)) != std::string::npos)
		{
			result.replace(position, placeholder.length(), value.second);
			position += value.second.length();
		}
	}
	return result;
}

// Builds the overload signature portion used by Object and GlobalObject bridge names.
// Example:
// Void_Vec3
std::string BuildBridgeSignature(MethodInfo const& methodInfo)
{
	std::string signature = GetBridgeTypeName(methodInfo.m_returnType);

	for (Variant::Type argumentType : methodInfo.m_argumentTypes)
	{
		signature += "_";
		signature += GetBridgeTypeName(argumentType);
	}

	return signature;
}

// Builds the user argument tail for a bridge declaration.
// Example:
// , const Vec3 &in arg0
std::string BuildBridgeDeclarationArguments(MethodInfo const& methodInfo)
{
	std::string arguments;
	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		arguments += ", ";
		arguments += BuildScriptArgumentDeclaration(methodInfo.m_argumentTypes[argumentIndex]);
		arguments += " arg";
		arguments += std::to_string(argumentIndex);
	}
	return arguments;
}

char const* GetBridgeNameTemplate(ScriptCallableKind kind)
{
	switch (kind)
	{
	case ScriptCallableKind::Object:
		return kObjectBridgeNameTemplate;
	case ScriptCallableKind::GlobalObject:
		return kGlobalObjectBridgeNameTemplate;
	case ScriptCallableKind::Global:
		return kGlobalBridgeNameTemplate;
	default:
		return kObjectBridgeNameTemplate;
	}
}

char const* GetBridgeDeclarationTemplate(ScriptCallableKind kind)
{
	switch (kind)
	{
	case ScriptCallableKind::Object:
		return kObjectBridgeDeclarationTemplate;
	case ScriptCallableKind::GlobalObject:
		return kGlobalObjectBridgeDeclarationTemplate;
	case ScriptCallableKind::Global:
		return kGlobalBridgeDeclarationTemplate;
	default:
		return kObjectBridgeDeclarationTemplate;
	}
}

} // namespace

std::string GetScriptTypeName(Variant::Type type)
{
	switch (type)
	{
	case Variant::Type::Empty:
		return "void";
	case Variant::Type::Bool:
		return "bool";
	case Variant::Type::Int:
		return "int";
	case Variant::Type::Float:
		return "float";
	case Variant::Type::String:
		return "string";
	case Variant::Type::Vec2:
		return "Vec2";
	case Variant::Type::Vec3:
		return "Vec3";
	case Variant::Type::Vec4:
		return "Vec4";
	case Variant::Type::AABB2:
		return "AABB2";
	case Variant::Type::OBB2:
		return "OBB2";
	case Variant::Type::Capsule3:
		return "Capsule3";
	case Variant::Type::EulerAngles:
		return "EulerAngles";
	case Variant::Type::Matrix4x4:
		return "Matrix4x4";
	case Variant::Type::ObjectPtr:
		return "NativeObject@";
	case Variant::Type::Any:
		return "Variant";
	default:
		return "unknown";
	}
}

std::string GetBridgeTypeName(Variant::Type type)
{
	switch (type)
	{
	case Variant::Type::Empty:
		return "Void";
	case Variant::Type::Bool:
		return "Bool";
	case Variant::Type::Int:
		return "Int";
	case Variant::Type::Float:
		return "Float";
	case Variant::Type::String:
		return "String";
	case Variant::Type::Vec2:
		return "Vec2";
	case Variant::Type::Vec3:
		return "Vec3";
	case Variant::Type::Vec4:
		return "Vec4";
	case Variant::Type::AABB2:
		return "AABB2";
	case Variant::Type::OBB2:
		return "OBB2";
	case Variant::Type::Capsule3:
		return "Capsule3";
	case Variant::Type::EulerAngles:
		return "EulerAngles";
	case Variant::Type::Matrix4x4:
		return "Matrix4x4";
	case Variant::Type::ObjectPtr:
		return "NativeObject";
	case Variant::Type::Any:
		return "Variant";
	default:
		return "Unknown";
	}
}

bool IsScriptRefType(Variant::Type type)
{
	return type == Variant::Type::String || type == Variant::Type::Vec2 || type == Variant::Type::Vec3
		   || type == Variant::Type::Vec4 || type == Variant::Type::AABB2 || type == Variant::Type::OBB2
		   || type == Variant::Type::Capsule3 || type == Variant::Type::EulerAngles || type == Variant::Type::Matrix4x4
		   || type == Variant::Type::Any;
}

std::string BuildScriptArgumentDeclaration(Variant::Type type)
{
	std::string declaration;
	if (IsScriptRefType(type))
	{
		declaration += "const ";
	}

	declaration += GetScriptTypeName(type);

	if (IsScriptRefType(type))
	{
		declaration += " &in";
	}

	return declaration;
}

std::string BuildBridgeFunctionName(MethodInfo const& methodInfo, ScriptCallableKind kind)
{
	return ExpandTemplate(
		GetBridgeNameTemplate(kind),
		{
			{ "signature", BuildBridgeSignature(methodInfo) },
			{ "methodName", methodInfo.m_name },
		});
}

std::string BuildBridgeFunctionDeclaration(MethodInfo const& methodInfo, ScriptCallableKind kind)
{
	return ExpandTemplate(
		GetBridgeDeclarationTemplate(kind),
		{
			{ "returnType", GetScriptTypeName(methodInfo.m_returnType) },
			{ "functionName", BuildBridgeFunctionName(methodInfo, kind) },
			{ "arguments", BuildBridgeDeclarationArguments(methodInfo) },
		});
}
