#include "MingEngine/Engine/Script/ScriptTypeUtils.hpp"

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
	case Variant::Type::Vec3:
		return "Vec3";
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
	case Variant::Type::Vec3:
		return "Vec3";
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
	return type == Variant::Type::String || type == Variant::Type::Vec3 || type == Variant::Type::EulerAngles
		   || type == Variant::Type::Matrix4x4 || type == Variant::Type::Any;
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

std::string BuildBridgeFunctionName(MethodInfo const& methodInfo, bool isGlobal)
{
	std::string functionName = "__Call_";
	if (isGlobal)
	{
		functionName += "GlobalObject_";
	}
	functionName += GetBridgeTypeName(methodInfo.m_returnType);

	for (Variant::Type argumentType : methodInfo.m_argumentTypes)
	{
		functionName += "_";
		functionName += GetBridgeTypeName(argumentType);
	}

	return functionName;
}

std::string BuildGlobalBridgeFunctionName(MethodInfo const& methodInfo)
{
	std::string functionName = "__Call_";
	functionName += "Global_";
	functionName += methodInfo.m_name;

	return functionName;
}

std::string BuildBridgeFunctionDeclaration(MethodInfo const& methodInfo, bool isGlobalObject)
{
	std::string declaration;
	if (isGlobalObject)
	{
		declaration = GetScriptTypeName(methodInfo.m_returnType) + " " + BuildBridgeFunctionName(methodInfo, true)
					  + "(const string &in className, const string &in methodName";
	}
	else
	{
		declaration = GetScriptTypeName(methodInfo.m_returnType) + " " + BuildBridgeFunctionName(methodInfo, false)
					  + "(NativeObject@ nativePtr, const string &in className, const string &in methodName";
	}

	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		declaration += ", ";
		declaration += BuildScriptArgumentDeclaration(methodInfo.m_argumentTypes[argumentIndex]);
		declaration += " arg";
		declaration += std::to_string(argumentIndex);
	}
	declaration += ")";

	return declaration;
}

std::string BuildGlobalBridgeFunctionDeclaration(MethodInfo const& methodInfo)
{
	std::string declaration = GetScriptTypeName(methodInfo.m_returnType) + " "
							  + BuildGlobalBridgeFunctionName(methodInfo)
							  + "const string &in namespaceName, const string &in methodName";

	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		declaration += ", ";
		declaration += BuildScriptArgumentDeclaration(methodInfo.m_argumentTypes[argumentIndex]);
		declaration += " arg";
		declaration += std::to_string(argumentIndex);
	}
	declaration += ")";

	return declaration;
}
