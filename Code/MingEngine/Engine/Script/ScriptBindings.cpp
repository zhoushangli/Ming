#include "MingEngine/Engine/Script/ScriptBindings.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/StringUtils.hpp"

#include "ThirdParty/angelscript/include/angelscript.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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

// ------------------------------------------------------------------------------------------------

namespace
{
constexpr char const* NATIVE_OBJECT_TYPE_NAME     = "NativeObject";
constexpr char const* NATIVE_OBJECT_PROPERTY_NAME = "nativePtr";

struct BridgeSignature
{
	Variant::Type              returnType = Variant::Type::Empty;
	std::vector<Variant::Type> argumentTypes;
};

std::unordered_map<int, BridgeSignature> s_bridgeSignatures;

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
	default:
		return "Unknown";
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

void GeneratePredefinedBuiltinTypes(std::string& outScript)
{
	outScript += "class NativeObject\n{\n}\n\n";

	outScript += "class string\n{\n";
	outScript += "\tstring();\n";
	outScript += "\tstring(const string &in arg0);\n\n";
	outScript += "\tstring &opAssign(const string &in arg0);\n";
	outScript += "\tstring &opAddAssign(const string &in arg0);\n";
	outScript += "\tbool opEquals(const string &in arg0) const;\n";
	outScript += "\tint opCmp(const string &in arg0) const;\n";
	outScript += "\tstring opAdd(const string &in arg0) const;\n\n";
	outScript += "\tuint length() const;\n";
	outScript += "\tvoid resize(uint arg0);\n";
	outScript += "\tuint get_length() const property;\n";
	outScript += "\tvoid set_length(uint arg0) property;\n";
	outScript += "\tbool isEmpty() const;\n";
	outScript += "\tstring substr(uint start = 0, int count = -1) const;\n";
	outScript += "\tint findFirst(const string &in arg0, uint start = 0) const;\n";
	outScript += "\tint findLast(const string &in arg0, int start = -1) const;\n";
	outScript += "}\n\n";

	outScript += "class Vec3\n{\n";
	outScript += "\tfloat x;\n";
	outScript += "\tfloat y;\n";
	outScript += "\tfloat z;\n\n";
	outScript += "\tVec3();\n";
	outScript += "\tVec3(float arg0, float arg1, float arg2);\n";
	outScript += "\tVec3(const Vec3 &in arg0);\n";
	outScript += "\tVec3 &opAssign(const Vec3 &in arg0);\n";
	outScript += "}\n\n";

	outScript += "class EulerAngles\n{\n";
	outScript += "\tfloat yawDegrees;\n";
	outScript += "\tfloat pitchDegrees;\n";
	outScript += "\tfloat rollDegrees;\n\n";
	outScript += "\tEulerAngles();\n";
	outScript += "\tEulerAngles(float arg0, float arg1, float arg2);\n";
	outScript += "\tEulerAngles(const EulerAngles &in arg0);\n";
	outScript += "\tEulerAngles &opAssign(const EulerAngles &in arg0);\n";
	outScript += "}\n\n";

	outScript += "class Matrix4x4\n{\n";
	outScript += "\tMatrix4x4();\n";
	outScript += "\tMatrix4x4(const Matrix4x4 &in arg0);\n";
	outScript += "\tMatrix4x4 &opAssign(const Matrix4x4 &in arg0);\n";
	outScript += "}\n\n";
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

std::string BuildBridgeFunctionDeclaration(MethodInfo const& methodInfo)
{
	std::string declaration = GetScriptTypeName(methodInfo.m_returnType) + " " + BuildBridgeFunctionName(methodInfo)
							  + "(NativeObject@ nativePtr, const string &in className, const string &in methodName";
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

void BridgeCallGeneric(asIScriptGeneric* gen)
{
	asIScriptFunction* function   = gen->GetFunction();
	int                functionId = function->GetId();

	auto iter = s_bridgeSignatures.find(functionId);
	GUARANTEE_OR_DIE(iter != s_bridgeSignatures.end(), "Missing bridge signature");
	BridgeSignature const& signature = iter->second;

	Object*            object     = static_cast<Object*>(gen->GetArgObject(0));
	std::string const& className  = *static_cast<std::string const*>(gen->GetArgAddress(1));
	std::string const& methodName = *static_cast<std::string const*>(gen->GetArgAddress(2));
	MethodBind const*  methodBind = ClassDatabase::GetMethodBind(className, methodName);

	// Parse arguments according to the signature map
	std::vector<Variant> args;
	for (size_t i = 0; i < signature.argumentTypes.size(); ++i)
	{
		int index = 3 + (int)i;
		switch (signature.argumentTypes[i])
		{
		case Variant::Type::Bool:
			args.emplace_back(gen->GetArgByte(index) != 0);
			break;
		case Variant::Type::Int:
			args.emplace_back(static_cast<int>(gen->GetArgDWord(index)));
			break;
		case Variant::Type::Float:
			args.emplace_back(gen->GetArgFloat(index));
			break;
		case Variant::Type::String:
			args.emplace_back(*static_cast<std::string*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Vec3:
			args.emplace_back(*static_cast<Vec3*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::EulerAngles:
			args.emplace_back(*static_cast<EulerAngles*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Matrix4x4:
			args.emplace_back(*static_cast<Matrix4x4*>(gen->GetArgAddress(index)));
			break;
		}
	}

	// Return value
	Variant result = methodBind->Invoke(*object, args);
	switch (signature.returnType)
	{
	case Variant::Type::Empty:
		return;

	case Variant::Type::Bool:
		gen->SetReturnByte(result.As<bool>() ? 1 : 0);
		return;

	case Variant::Type::Int:
		gen->SetReturnDWord(static_cast<asDWORD>(result.As<int>()));
		return;

	case Variant::Type::Float:
		gen->SetReturnFloat(result.As<float>());
		return;

	case Variant::Type::String:
		gen->SetReturnObject(const_cast<std::string*>(&result.As<std::string>()));
		return;

	case Variant::Type::Vec3:
	{
		Vec3 const& vecResult = result.As<Vec3>();
		gen->SetReturnObject(const_cast<Vec3*>(&vecResult));
		return;
	}

	case Variant::Type::EulerAngles:
	{
		EulerAngles const& eulerResult = result.As<EulerAngles>();
		gen->SetReturnObject(const_cast<EulerAngles*>(&eulerResult));
		return;
	}

	case Variant::Type::Matrix4x4:
	{
		Matrix4x4 const& matrixResult = result.As<Matrix4x4>();
		gen->SetReturnObject(const_cast<Matrix4x4*>(&matrixResult));
		return;
	}

	default:
		GUARANTEE_OR_DIE(false, "Unsupported generic bridge return type");
		return;
	}
}

} // namespace

void RegisterNativeObjectType(asIScriptEngine* engine)
{
	int result = engine->RegisterObjectType(NATIVE_OBJECT_TYPE_NAME, 0, asOBJ_REF | asOBJ_NOCOUNT);
	GUARANTEE_OR_DIE(result >= 0, Stringf("Failed to register script class: %s", NATIVE_OBJECT_TYPE_NAME));
}

void RegisterBridgeFunctions(asIScriptEngine* engine)
{
	std::vector<ClassInfo const*>   classes = ClassDatabase::GetRegisteredClasses();
	std::unordered_set<std::string> registeredFunctions;
	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr)
		{
			continue;
		}

		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo == nullptr)
			{
				continue;
			}

			std::string scriptDeclaration = BuildBridgeFunctionDeclaration(*methodInfo);
			if (registeredFunctions.find(scriptDeclaration) != registeredFunctions.end())
			{
				continue;
			}

			int functionId = engine->RegisterGlobalFunction(
				scriptDeclaration.c_str(),
				asFUNCTION(BridgeCallGeneric),
				asCALL_GENERIC);
			GUARANTEE_OR_DIE(
				functionId >= 0,
				Stringf("Failed to register bridge function: %s", scriptDeclaration.c_str()));

			registeredFunctions.insert(scriptDeclaration);
			s_bridgeSignatures[functionId] = { methodInfo->m_returnType, methodInfo->m_argumentTypes };
		}
	}
}

void GenerateBuiltinScript()
{
	std::string scriptText;
	std::string predefinedText;

	ClassInfo const* objectClassInfo = ClassDatabase::GetClassInfo("Object");
	GenerateRootObjectClass(objectClassInfo, scriptText);

	// Generate the predefined script
	GeneratePredefinedBuiltinTypes(predefinedText);
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

	std::filesystem::create_directories("Data/Scripts");

	std::ofstream scriptFile("Data/Scripts/MingEngine.generated.as");
	GUARANTEE_OR_DIE(scriptFile.is_open(), "Failed to open Data/Scripts/MingEngine.generated.as for writing.");

	scriptFile << scriptText;
	GUARANTEE_OR_DIE(!scriptFile.bad(), "Failed to write Data/Scripts/MingEngine.generated.as.");

	std::ofstream predefinedFile("Data/Scripts/as.predefined");
	GUARANTEE_OR_DIE(predefinedFile.is_open(), "Failed to open Data/Scripts/as.predefined for writing.");

	predefinedFile << predefinedText;
	GUARANTEE_OR_DIE(!predefinedFile.bad(), "Failed to write Data/Scripts/as.predefined.");
}
