#include "MingEngine/Engine/Script/ScriptGenerator.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Script/ScriptTypeUtils.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
enum class ScriptBuildMode
{
	RuntimeWrapper,
	PredefinedDeclaration,
};

// Script wrapper templates.
// These generate AngelScript source that forwards script calls to native bridge functions.
// Method builders produce `${methods}` first, then container builders insert it into class/namespace templates.

// Example:
// class Object
// {
//     protected NativeObject@ nativePtr;
//     ...
// }
constexpr char const* kRootObjectClassTemplate =
R"AS(
class Object
{
	protected ${nativeObjectType}@ ${nativeObjectProperty};

${methods}}

)AS";

// Example:
// class Node3D : Node
// {
//     void SetPosition(const Vec3 &in arg0) { ... }
// }
constexpr char const* kObjectClassTemplate =
R"AS(
class ${className}${inheritance}
{
${methods}}

)AS";

// Used by both GlobalObject and Global script APIs.
// Example:
// namespace InputSystem { ... }
// namespace Debug { ... }
constexpr char const* kNamespaceTemplate =
R"AS(
namespace ${namespaceName}
{
${methods}}

)AS";

// Object method wrapper.
// Example:
// void SetPosition(const Vec3 &in arg0) { __Call_Void_Vec3(nativePtr, "Node3D", "SetPosition", arg0); }
constexpr char const* kObjectMethodTemplate =
R"AS(
	${returnType} ${methodName}(${arguments})${constSuffix}
	{
		${returnPrefix}${bridgeName}(${nativeObjectProperty}, "${className}", "${methodName}"${callArguments});
	}

)AS";

// GlobalObject method wrapper.
// Example:
// bool IsKeyPressed(int arg0) { return __Call_GlobalObject_Bool_Int("InputSystem", "IsKeyPressed", arg0); }
constexpr char const* kGlobalObjectMethodTemplate =
R"AS(
	${returnType} ${methodName}(${arguments})${constSuffix}
	{
		${returnPrefix}${bridgeName}("${className}", "${methodName}"${callArguments});
	}

)AS";

// Global method wrapper.
// Example:
// void Log(const string &in arg0) { __Call_Global_Log("Debug", "Log", arg0); }
constexpr char const* kGlobalMethodTemplate =
R"AS(
	${returnType} ${methodName}(${arguments})${constSuffix}
	{
		${returnPrefix}${bridgeName}("${namespaceName}", "${methodName}"${callArguments});
	}

)AS";

// Predefined declaration used by as.predefined.
// Example:
// void SetPosition(const Vec3 &in arg0);
constexpr char const* kMethodDeclarationTemplate =
R"AS(
	${returnType} ${methodName}(${arguments})${constSuffix};
)AS";

// Keeps raw string templates readable while avoiding an extra blank line in generated scripts.
std::string TrimTemplateText(char const* templateText)
{
	std::string text = templateText;
	if (!text.empty() && text.front() == '\n')
	{
		text.erase(text.begin());
	}
	return text;
}

// Replaces simple `${name}` placeholders in a script template.
std::string ExpandTemplate(char const* templateText, std::unordered_map<std::string, std::string> const& values)
{
	std::string result = TrimTemplateText(templateText);
	for (std::pair<std::string const, std::string> const& value : values)
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

std::string BuildMethodArguments(MethodInfo const& methodInfo)
{
	std::string arguments;
	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		if (argumentIndex > 0)
		{
			arguments += ", ";
		}

		Variant::Type argumentType = methodInfo.m_argumentTypes[argumentIndex];
		arguments += BuildScriptArgumentDeclaration(argumentType);
		arguments += " arg";
		arguments += std::to_string(argumentIndex);
	}
	return arguments;
}

std::string BuildCallArguments(MethodInfo const& methodInfo)
{
	std::string arguments;
	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentTypes.size(); ++argumentIndex)
	{
		arguments += ", arg";
		arguments += std::to_string(argumentIndex);
	}
	return arguments;
}

std::string BuildConstSuffix(MethodInfo const& methodInfo)
{
	return methodInfo.m_isConst ? " const" : "";
}

std::string BuildReturnPrefix(MethodInfo const& methodInfo)
{
	return methodInfo.m_returnType != Variant::Type::Empty ? "return " : "";
}

std::string BuildInheritanceSuffix(ClassInfo const& classInfo)
{
	if (classInfo.m_parentClassName.empty())
	{
		return "";
	}

	return " : " + classInfo.m_parentClassName;
}

std::unordered_map<std::string, std::string> BuildMethodTemplateValues(MethodInfo const& methodInfo)
{
	return {
		{ "returnType", GetScriptTypeName(methodInfo.m_returnType) },
		{ "methodName", methodInfo.m_name },
		{ "arguments", BuildMethodArguments(methodInfo) },
		{ "constSuffix", BuildConstSuffix(methodInfo) },
		{ "returnPrefix", BuildReturnPrefix(methodInfo) },
		{ "callArguments", BuildCallArguments(methodInfo) },
	};
}

// Builds a method signature without a body.
// Example:
// void SetPosition(const Vec3 &in arg0);
std::string BuildMethodDeclarationScript(MethodInfo const& methodInfo)
{
	return ExpandTemplate(kMethodDeclarationTemplate, BuildMethodTemplateValues(methodInfo));
}

// Builds either an Object runtime wrapper or its predefined declaration.
// RuntimeWrapper example:
// void SetPosition(const Vec3 &in arg0) { __Call_Void_Vec3(nativePtr, "Node3D", "SetPosition", arg0); }
// PredefinedDeclaration example:
// void SetPosition(const Vec3 &in arg0);
std::string BuildObjectMethodScript(ClassInfo const& classInfo, MethodInfo const& methodInfo, ScriptBuildMode mode)
{
	if (mode == ScriptBuildMode::PredefinedDeclaration)
	{
		return BuildMethodDeclarationScript(methodInfo);
	}

	std::unordered_map<std::string, std::string> values = BuildMethodTemplateValues(methodInfo);
	values["bridgeName"]                           = BuildBridgeFunctionName(methodInfo, ScriptCallableKind::Object);
	values["nativeObjectProperty"]                 = kNativeObjectPropertyName;
	values["className"]                            = classInfo.m_className;
	return ExpandTemplate(kObjectMethodTemplate, values);
}

// Builds either a GlobalObject runtime wrapper or its predefined declaration.
// RuntimeWrapper example:
// bool IsKeyPressed(int arg0) { return __Call_GlobalObject_Bool_Int("InputSystem", "IsKeyPressed", arg0); }
// PredefinedDeclaration example:
// bool IsKeyPressed(int arg0);
std::string BuildGlobalObjectMethodScript(ClassInfo const& classInfo, MethodInfo const& methodInfo, ScriptBuildMode mode)
{
	if (mode == ScriptBuildMode::PredefinedDeclaration)
	{
		return BuildMethodDeclarationScript(methodInfo);
	}

	std::unordered_map<std::string, std::string> values = BuildMethodTemplateValues(methodInfo);
	values["bridgeName"]                           = BuildBridgeFunctionName(methodInfo, ScriptCallableKind::GlobalObject);
	values["className"]                            = classInfo.m_className;
	return ExpandTemplate(kGlobalObjectMethodTemplate, values);
}

// Builds either a Global runtime wrapper or its predefined declaration.
// RuntimeWrapper example:
// void Log(const string &in arg0) { __Call_Global_Log("Debug", "Log", arg0); }
// PredefinedDeclaration example:
// void Log(const string &in arg0);
std::string BuildGlobalMethodScript(
	GlobalNamespaceInfo const& globalNamespace, MethodInfo const& methodInfo, ScriptBuildMode mode)
{
	if (mode == ScriptBuildMode::PredefinedDeclaration)
	{
		return BuildMethodDeclarationScript(methodInfo);
	}

	std::unordered_map<std::string, std::string> values = BuildMethodTemplateValues(methodInfo);
	values["bridgeName"]                           = BuildBridgeFunctionName(methodInfo, ScriptCallableKind::Global);
	values["namespaceName"]                        = globalNamespace.m_namespaceName;
	return ExpandTemplate(kGlobalMethodTemplate, values);
}

// Builds the root Object class that stores the native object pointer.
// Example:
// class Object { protected NativeObject@ nativePtr; ... }
std::string BuildRootObjectScript(ClassInfo const* classInfo, ScriptBuildMode mode)
{
	std::string methods;
	if (classInfo != nullptr)
	{
		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo != nullptr)
			{
				methods += BuildObjectMethodScript(*classInfo, *methodInfo, mode);
			}
		}
	}

	return ExpandTemplate(
		kRootObjectClassTemplate,
		{
			{ "nativeObjectType", kNativeObjectTypeName },
			{ "nativeObjectProperty", kNativeObjectPropertyName },
			{ "methods", methods },
		});
}

// Object: script-visible class methods that require an instance.
// Example:
// class Node3D { void SetPosition(const Vec3 &in arg0) { __Call_Void_Vec3(nativePtr, "Node3D", "SetPosition", arg0); } }
std::string BuildObjectScript(ClassInfo const& classInfo, ScriptBuildMode mode);

// GlobalObject: script-visible namespace functions backed by a registered global object.
// Example:
// namespace InputSystem { bool IsKeyPressed(int arg0) { return __Call_GlobalObject_Bool_Int("InputSystem", "IsKeyPressed", arg0); } }
std::string BuildGlobalObjectScript(ClassInfo const& classInfo, ScriptBuildMode mode)
{
	std::string methods;
	for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo.m_methods)
	{
		if (methodInfo != nullptr)
		{
			methods += BuildGlobalObjectMethodScript(classInfo, *methodInfo, mode);
		}
	}

	return ExpandTemplate(
		kNamespaceTemplate,
		{
			{ "namespaceName", classInfo.m_className },
			{ "methods", methods },
		});
}

// Builds a script-visible class for Object-derived types that need an instance.
// If the class derives from SystemBase, it is emitted as a GlobalObject namespace instead.
// Example:
// class Node3D : Node { void SetPosition(const Vec3 &in arg0) { __Call_Void_Vec3(nativePtr, "Node3D", "SetPosition", arg0); } }
std::string BuildObjectScript(ClassInfo const& classInfo, ScriptBuildMode mode)
{
	if (classInfo.m_className == "Object")
	{
		return "";
	}

	if (classInfo.m_parentClassName == SystemBase::GetStaticClassName())
	{
		return BuildGlobalObjectScript(classInfo, mode);
	}

	std::string methods;
	for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo.m_methods)
	{
		if (methodInfo != nullptr)
		{
			methods += BuildObjectMethodScript(classInfo, *methodInfo, mode);
		}
	}

	return ExpandTemplate(
		kObjectClassTemplate,
		{
			{ "className", classInfo.m_className },
			{ "inheritance", BuildInheritanceSuffix(classInfo) },
			{ "methods", methods },
		});
}

// Global: script-visible namespace functions backed by native free functions.
// Example:
// namespace Debug { void Log(const string &in arg0) { __Call_Global_Log("Debug", "Log", arg0); } }
std::string BuildGlobalScript(GlobalNamespaceInfo const& globalNamespace, ScriptBuildMode mode)
{
	std::string methods;
	for (std::unique_ptr<MethodInfo> const& methodInfo : globalNamespace.m_methods)
	{
		if (methodInfo != nullptr)
		{
			methods += BuildGlobalMethodScript(globalNamespace, *methodInfo, mode);
		}
	}

	return ExpandTemplate(
		kNamespaceTemplate,
		{
			{ "namespaceName", globalNamespace.m_namespaceName },
			{ "methods", methods },
		});
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
	scriptText += BuildRootObjectScript(objectClassInfo, ScriptBuildMode::RuntimeWrapper);

	// Generate the predefined script
	GeneratePredefinedBuiltinTypes(engine, predefinedText);
	predefinedText += BuildRootObjectScript(objectClassInfo, ScriptBuildMode::PredefinedDeclaration);

	std::vector<GlobalNamespaceInfo const*> globalNamespaces = ClassDatabase::GetRegisteredGlobalNamespaces();
	std::vector<ClassInfo const*>           classes          = ClassDatabase::GetRegisteredClasses();
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

	for (GlobalNamespaceInfo const* globalNamespace : globalNamespaces)
	{
		if (globalNamespace == nullptr)
		{
			continue;
		}

		scriptText += BuildGlobalScript(*globalNamespace, ScriptBuildMode::RuntimeWrapper);
		predefinedText += BuildGlobalScript(*globalNamespace, ScriptBuildMode::PredefinedDeclaration);
	}

	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr || classInfo->m_className == "Object")
		{
			continue;
		}

		scriptText += BuildObjectScript(*classInfo, ScriptBuildMode::RuntimeWrapper);
		predefinedText += BuildObjectScript(*classInfo, ScriptBuildMode::PredefinedDeclaration);
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
