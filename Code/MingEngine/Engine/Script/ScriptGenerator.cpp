#include "MingEngine/Engine/Script/ScriptGenerator.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr char const* NATIVE_OBJECT_TYPE_NAME     = "NativeObject";
constexpr char const* NATIVE_OBJECT_PROPERTY_NAME = "nativePtr";

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

bool IsScriptRefType(Variant::Type type)
{
	return type == Variant::Type::String || type == Variant::Type::Vec3 || type == Variant::Type::EulerAngles
		   || type == Variant::Type::Matrix4x4;
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

std::string BuildBridgeFunctionName(MethodInfo const& methodInfo)
{
	std::string functionName = "__Call_";
	functionName += GetBridgeTypeName(methodInfo.m_returnType);

	for (Variant::Type argumentType : methodInfo.m_argumentTypes)
	{
		functionName += "_";
		functionName += GetBridgeTypeName(argumentType);
	}

	return functionName;
}
void GenerateMethodArguments(MethodInfo const& methodInfo, std::string& outScript)
{
	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		if (argumentIndex > 0)
		{
			outScript += ", ";
		}

		Variant::Type argumentType = methodInfo.m_argumentTypes[argumentIndex];
		outScript += BuildScriptArgumentDeclaration(argumentType);
		outScript += " arg";
		outScript += std::to_string(argumentIndex);
	}
}

void GenerateMethod(ClassInfo const& classInfo, MethodInfo const& methodInfo, std::string& outScript)
{
	outScript += "\t";
	outScript += GetScriptTypeName(methodInfo.m_returnType);
	outScript += " ";
	outScript += methodInfo.m_name;
	outScript += "(";
	GenerateMethodArguments(methodInfo, outScript);
	outScript += ")";
	if (methodInfo.m_isConst)
	{
		outScript += " const";
	}
	outScript += "\n\t{\n\t\t";
	if (methodInfo.m_returnType != Variant::Type::Empty)
	{
		outScript += "return ";
	}
	outScript += BuildBridgeFunctionName(methodInfo);
	outScript += "(";
	outScript += NATIVE_OBJECT_PROPERTY_NAME;
	outScript += ", \"";
	outScript += classInfo.m_className;
	outScript += "\", \"";
	outScript += methodInfo.m_name;
	outScript += "\"";

	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		outScript += ", arg";
		outScript += std::to_string(argumentIndex);
	}

	outScript += ");\n\t}\n\n";
}

void GenerateRootObjectClass(ClassInfo const* classInfo, std::string& outScript)
{
	outScript += "class Object\n{\n";
	outScript += "\tprotected ";
	outScript += NATIVE_OBJECT_TYPE_NAME;
	outScript += "@ ";
	outScript += NATIVE_OBJECT_PROPERTY_NAME;
	outScript += ";\n\n";

	if (classInfo != nullptr)
	{
		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo != nullptr)
			{
				GenerateMethod(*classInfo, *methodInfo, outScript);
			}
		}
	}

	outScript += "}\n\n";
}

void GenerateClass(ClassInfo const& classInfo, std::string& outScript)
{
	if (classInfo.m_className == "Object")
	{
		return;
	}

	outScript += "class ";
	outScript += classInfo.m_className;

	if (!classInfo.m_parentClassName.empty())
	{
		outScript += " : ";
		outScript += classInfo.m_parentClassName;
	}

	outScript += "\n{\n";
	for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo.m_methods)
	{
		if (methodInfo != nullptr)
		{
			GenerateMethod(classInfo, *methodInfo, outScript);
		}
	}
	outScript += "}\n\n";
}

void GeneratePredefinedBuiltinTypes(asIScriptEngine* engine, std::string& outScript)
{
	if (engine == nullptr)
	{
		return;
	}

	for (asUINT typeIndex = 0; typeIndex < engine->GetObjectTypeCount(); ++typeIndex)
	{
		asITypeInfo* typeInfo = engine->GetObjectTypeByIndex(typeIndex);
		if (typeInfo == nullptr)
		{
			continue;
		}

		outScript += "class ";
		outScript += typeInfo->GetName();
		outScript += "\n{\n";

		for (asUINT behaviorIndex = 0; behaviorIndex < typeInfo->GetBehaviourCount(); ++behaviorIndex)
		{
			asEBehaviours      behavior = asBEHAVE_CONSTRUCT;
			asIScriptFunction* function = typeInfo->GetBehaviourByIndex(behaviorIndex, &behavior);
			if (function != nullptr && (behavior == asBEHAVE_CONSTRUCT || behavior == asBEHAVE_DESTRUCT))
			{
				outScript += "\t";
				outScript += function->GetDeclaration(false, true, true);
				outScript += ";\n";
			}
		}

		for (asUINT methodIndex = 0; methodIndex < typeInfo->GetMethodCount(); ++methodIndex)
		{
			asIScriptFunction* method = typeInfo->GetMethodByIndex(methodIndex);
			if (method != nullptr)
			{
				outScript += "\t";
				outScript += method->GetDeclaration(false, true, true);
				outScript += ";\n";
			}
		}

		for (asUINT propertyIndex = 0; propertyIndex < typeInfo->GetPropertyCount(); ++propertyIndex)
		{
			outScript += "\t";
			outScript += typeInfo->GetPropertyDeclaration(propertyIndex, true);
			outScript += ";\n";
		}

		for (asUINT funcdefIndex = 0; funcdefIndex < typeInfo->GetChildFuncdefCount(); ++funcdefIndex)
		{
			asITypeInfo* funcdef = typeInfo->GetChildFuncdef(funcdefIndex);
			if (funcdef != nullptr && funcdef->GetFuncdefSignature() != nullptr)
			{
				outScript += "\tfuncdef ";
				outScript += funcdef->GetFuncdefSignature()->GetDeclaration(false);
				outScript += ";\n";
			}
		}

		outScript += "}\n\n";
	}
}

void GeneratePredefinedMethod(MethodInfo const& methodInfo, std::string& outScript)
{
	outScript += "\t";
	outScript += GetScriptTypeName(methodInfo.m_returnType);
	outScript += " ";
	outScript += methodInfo.m_name;
	outScript += "(";
	GenerateMethodArguments(methodInfo, outScript);
	outScript += ")";
	if (methodInfo.m_isConst)
	{
		outScript += " const";
	}
	outScript += ";\n";
}

void GeneratePredefinedRootObjectClass(ClassInfo const* classInfo, std::string& outScript)
{
	outScript += "class Object\n{\n";
	outScript += "\tprotected ";
	outScript += NATIVE_OBJECT_TYPE_NAME;
	outScript += "@ ";
	outScript += NATIVE_OBJECT_PROPERTY_NAME;
	outScript += ";\n\n";

	if (classInfo != nullptr)
	{
		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo != nullptr)
			{
				GeneratePredefinedMethod(*methodInfo, outScript);
			}
		}
	}

	outScript += "}\n\n";
}

void GeneratePredefinedClass(ClassInfo const& classInfo, std::string& outScript)
{
	if (classInfo.m_className == "Object")
	{
		return;
	}

	outScript += "class ";
	outScript += classInfo.m_className;

	if (!classInfo.m_parentClassName.empty())
	{
		outScript += " : ";
		outScript += classInfo.m_parentClassName;
	}

	outScript += "\n{\n";
	for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo.m_methods)
	{
		if (methodInfo != nullptr)
		{
			GeneratePredefinedMethod(*methodInfo, outScript);
		}
	}
	outScript += "}\n\n";
}

int GetClassDepth(ClassInfo const& classInfo)
{
	int         depth           = 0;
	std::string parentClassName = classInfo.m_parentClassName;

	while (!parentClassName.empty())
	{
		ClassInfo const* parentClassInfo = ClassDatabase::GetClassInfo(parentClassName);
		if (parentClassInfo == nullptr)
		{
			break;
		}

		++depth;
		parentClassName = parentClassInfo->m_parentClassName;
	}

	return depth;
}

} // namespace

void GenerateBuiltinScript(asIScriptEngine* engine)
{
	std::string scriptText;
	std::string predefinedText;

	ClassInfo const* objectClassInfo = ClassDatabase::GetClassInfo("Object");
	GenerateRootObjectClass(objectClassInfo, scriptText);

	// Generate the predefined script
	GeneratePredefinedBuiltinTypes(engine, predefinedText);
	GeneratePredefinedRootObjectClass(objectClassInfo, predefinedText);

	std::vector<ClassInfo const*> classes = ClassDatabase::GetRegisteredClasses();
	std::sort(
		classes.begin(),
		classes.end(),
		[](ClassInfo const* left, ClassInfo const* right)
		{
			if (left == nullptr)
			{
				return false;
			}

			if (right == nullptr)
			{
				return true;
			}

			int leftDepth  = GetClassDepth(*left);
			int rightDepth = GetClassDepth(*right);
			if (leftDepth != rightDepth)
			{
				return leftDepth < rightDepth;
			}

			return left->m_className < right->m_className;
		});

	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo != nullptr && classInfo->m_className != "Object")
		{
			GenerateClass(*classInfo, scriptText);
			GeneratePredefinedClass(*classInfo, predefinedText);
		}
	}

	std::filesystem::create_directories(kScriptLibDirectory);

	std::ofstream scriptFile(std::filesystem::path(kScriptLibDirectory) / kGeneratedScriptFilename);
	GUARANTEE_OR_DIE(scriptFile.is_open(), "Failed to open Data/ScriptLib/MingEngine.generated.as for writing.");

	scriptFile << scriptText;
	GUARANTEE_OR_DIE(!scriptFile.bad(), "Failed to write Data/ScriptLib/MingEngine.generated.as.");

	std::ofstream predefinedFile(std::filesystem::path(kScriptLibDirectory) / kPredefinedScriptFilename);
	GUARANTEE_OR_DIE(predefinedFile.is_open(), "Failed to open Data/ScriptLib/as.predefined for writing.");

	predefinedFile << predefinedText;
	GUARANTEE_OR_DIE(!predefinedFile.bad(), "Failed to write Data/ScriptLib/as.predefined.");
}
