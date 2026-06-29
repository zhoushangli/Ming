#include "MingEngine/Engine/Script/ScriptBinder.hpp"

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
struct BridgeSignature
{
	Variant::Type             m_returnType = Variant::Type::Empty;
	std::vector<ArgumentInfo> m_argumentInfos;
};

std::unordered_map<int, BridgeSignature> s_bridgeSignatures;

// Looks up the Variant signature saved when the bridge function was registered.
// AngelScript generic calls only expose raw slots, so this signature tells us how to decode them.
BridgeSignature const& GetBridgeSignature(asIScriptGeneric* gen)
{
	asIScriptFunction* function   = gen->GetFunction();
	int                functionId = function->GetId();

	auto iter = s_bridgeSignatures.find(functionId);
	GUARANTEE_OR_DIE(iter != s_bridgeSignatures.end(), "Missing bridge signature");
	return iter->second;
}

// Converts AngelScript generic arguments into Variants.
// firstUserArgIndex skips fixed bridge arguments such as nativePtr/className/methodName.
std::vector<Variant> ReadBridgeArguments(asIScriptGeneric* gen, BridgeSignature const& signature, int firstUserArgIndex)
{
	std::vector<Variant> args;
	for (size_t i = 0; i < signature.m_argumentInfos.size(); ++i)
	{
		int index = firstUserArgIndex + (int)i;
		switch (signature.m_argumentInfos[i].m_type)
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
		case Variant::Type::Vec2:
			args.emplace_back(*static_cast<Vec2*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Vec3:
			args.emplace_back(*static_cast<Vec3*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Vec4:
			args.emplace_back(*static_cast<Vec4*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::AABB2:
			args.emplace_back(*static_cast<AABB2*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::OBB2:
			args.emplace_back(*static_cast<OBB2*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Capsule3:
			args.emplace_back(*static_cast<Capsule3*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::EulerAngles:
			args.emplace_back(*static_cast<EulerAngles*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Matrix4x4:
			args.emplace_back(*static_cast<Matrix4x4*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::Any:
			args.emplace_back(*static_cast<Variant*>(gen->GetArgAddress(index)));
			break;
		case Variant::Type::ObjectPtr:
			args.emplace_back(static_cast<Object*>(gen->GetArgObject(index)));
			break;
		}
	}
	return args;
}

// Writes a Variant return value back into the AngelScript generic call frame.
// Example:
// Variant::Type::Vec3 -> gen->SetReturnObject(Vec3*)
void WriteBridgeReturn(asIScriptGeneric* gen, Variant::Type returnType, Variant const& result)
{
	switch (returnType)
	{
	case Variant::Type::Empty:
	{
		return;
	}

	case Variant::Type::Bool:
	{
		gen->SetReturnByte(result.As<bool>() ? 1 : 0);
		return;
	}

	case Variant::Type::Int:
	{
		gen->SetReturnDWord(static_cast<asDWORD>(result.As<int>()));
		return;
	}

	case Variant::Type::Float:
	{
		gen->SetReturnFloat(result.As<float>());
		return;
	}

	case Variant::Type::String:
	{
		std::string const& resultString = result.As<std::string>();
		gen->SetReturnObject(const_cast<std::string*>(&resultString));
		return;
	}

	case Variant::Type::Vec2:
	{
		Vec2 const& vecResult = result.As<Vec2>();
		gen->SetReturnObject(const_cast<Vec2*>(&vecResult));
		return;
	}

	case Variant::Type::Vec3:
	{
		Vec3 const& vecResult = result.As<Vec3>();
		gen->SetReturnObject(const_cast<Vec3*>(&vecResult));
		return;
	}

	case Variant::Type::Vec4:
	{
		Vec4 const& vecResult = result.As<Vec4>();
		gen->SetReturnObject(const_cast<Vec4*>(&vecResult));
		return;
	}

	case Variant::Type::AABB2:
	{
		AABB2 const& boxResult = result.As<AABB2>();
		gen->SetReturnObject(const_cast<AABB2*>(&boxResult));
		return;
	}

	case Variant::Type::OBB2:
	{
		OBB2 const& boxResult = result.As<OBB2>();
		gen->SetReturnObject(const_cast<OBB2*>(&boxResult));
		return;
	}

	case Variant::Type::Capsule3:
	{
		Capsule3 const& capsuleResult = result.As<Capsule3>();
		gen->SetReturnObject(const_cast<Capsule3*>(&capsuleResult));
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

	// Variant::Type::Any is a special case where we are returning a Variant itself
	case Variant::Type::Any:
	{
		Variant const& variantResult = result;
		gen->SetReturnObject(const_cast<Variant*>(&variantResult));
		return;
	}

	// Variant::Type::ObjectPtr is a special case where we are returning a pointer to the Object
	case Variant::Type::ObjectPtr:
	{
		Object* objectResult = result.As<Object*>();
		gen->SetReturnObject(objectResult);
		return;
	}

	default:
		GUARANTEE_OR_DIE(false, "Unsupported generic bridge return type");
		return;
	}
}

// Shared invoke path for Object, GlobalObject, and Global bridge calls.
// The entry points only differ in how they resolve object/methodBind and firstUserArgIndex.
// Global calls pass nullptr because native free functions do not have a receiver object.
void InvokeBridgeMethod(
	asIScriptGeneric*      gen,
	BridgeSignature const& signature,
	Object*                object,
	MethodBind const&      methodBind,
	int                    firstUserArgIndex)
{
	std::vector<Variant> args   = ReadBridgeArguments(gen, signature, firstUserArgIndex);
	Variant              result = methodBind.Invoke(object, args);
	WriteBridgeReturn(gen, signature.m_returnType, result);
}

// Object bridge layout:
// arg0 = NativeObject@
// arg1 = className
// arg2 = methodName
// arg3... = user arguments
// Example call:
// __Call_Void_Vec3(nativePtr, "Node3D", "SetPosition", arg0)
void BridgeObjectGeneric(asIScriptGeneric* gen)
{
	BridgeSignature const& signature  = GetBridgeSignature(gen);
	Object*                object     = static_cast<Object*>(gen->GetArgObject(0));
	std::string const&     className  = *static_cast<std::string const*>(gen->GetArgAddress(1));
	std::string const&     methodName = *static_cast<std::string const*>(gen->GetArgAddress(2));
	MethodBind const*      methodBind = ClassDatabase::GetMethodBind(className, methodName);
	GUARANTEE_OR_DIE(object != nullptr, Stringf("Bridge object for class '%s' is null", className.c_str()));
	GUARANTEE_OR_DIE(
		methodBind != nullptr,
		Stringf("Method bind for '%s::%s' not found", className.c_str(), methodName.c_str()));
	InvokeBridgeMethod(gen, signature, object, *methodBind, 3);
}

// GlobalObject bridge layout:
// arg0 = className
// arg1 = methodName
// arg2... = user arguments
// Example call:
// __Call_GlobalObject_Bool_Int("InputSystem", "IsKeyPressed", arg0)
void BridgeGlobalObjectGeneric(asIScriptGeneric* gen)
{
	BridgeSignature const& signature  = GetBridgeSignature(gen);
	std::string const&     className  = *static_cast<std::string const*>(gen->GetArgAddress(0));
	std::string const&     methodName = *static_cast<std::string const*>(gen->GetArgAddress(1));
	MethodBind const*      methodBind = ClassDatabase::GetMethodBind(className, methodName);
	Object*                object     = ClassDatabase::GetGlobalObject(className);
	GUARANTEE_OR_DIE(
		methodBind != nullptr,
		Stringf("Global object method bind for '%s::%s' not found", className.c_str(), methodName.c_str()));
	if (object == nullptr)
	{
		GUARANTEE_OR_DIE(false, Stringf("Global object for class '%s' not found", className.c_str()));
	}

	InvokeBridgeMethod(gen, signature, object, *methodBind, 2);
}

// Global bridge layout:
// arg0 = namespaceName
// arg1 = methodName
// arg2... = user arguments
// Example call:
// __Call_Global_Log("Debug", "Log", arg0)
void BridgeGlobalGeneric(asIScriptGeneric* gen)
{
	BridgeSignature const& signature     = GetBridgeSignature(gen);
	std::string const&     namespaceName = *static_cast<std::string const*>(gen->GetArgAddress(0));
	std::string const&     methodName    = *static_cast<std::string const*>(gen->GetArgAddress(1));
	MethodBind const*      methodBind    = ClassDatabase::GetGlobalMethodBind(namespaceName, methodName);
	GUARANTEE_OR_DIE(
		methodBind != nullptr,
		Stringf("Global method bind for '%s::%s' not found", namespaceName.c_str(), methodName.c_str()));
	InvokeBridgeMethod(gen, signature, nullptr, *methodBind, 2);
}

Object* CreateNativeObject(std::string const& className)
{
	Object* object = ClassDatabase::CreateInstance(className);
	if (object == nullptr)
	{
		DebuggerPrintf("Script: failed to create native object '%s'.\n", className.c_str());
		return nullptr;
	}

	return object;
}

} // namespace

void RegisterNativeObjectType(asIScriptEngine* engine)
{
	int result = engine->RegisterObjectType(kNativeObjectTypeName, 0, asOBJ_REF | asOBJ_NOCOUNT);
	GUARANTEE_OR_DIE(result >= 0, Stringf("Failed to register script class: %s", kNativeObjectTypeName));
}

void RegisterBridgeFunctions(asIScriptEngine* engine)
{
	std::vector<GlobalNamespaceInfo const*> globalNamespaces = ClassDatabase::GetRegisteredGlobalNamespaces();
	for (GlobalNamespaceInfo const* globalNamespace : globalNamespaces)
	{
		if (globalNamespace == nullptr)
		{
			continue;
		}

		for (std::unique_ptr<MethodInfo> const& methodInfo : globalNamespace->m_methods)
		{
			if (methodInfo == nullptr)
			{
				continue;
			}

			std::string scriptDeclaration = BuildBridgeFunctionDeclaration(*methodInfo, ScriptCallableKind::Global);
			int         functionId        = engine->RegisterGlobalFunction(
				scriptDeclaration.c_str(),
				asFUNCTION(BridgeGlobalGeneric),
				asCALL_GENERIC);
			GUARANTEE_OR_DIE(
				functionId >= 0,
				Stringf("Failed to register bridge function: %s", scriptDeclaration.c_str()));

			s_bridgeSignatures[functionId] = { methodInfo->m_returnType, methodInfo->m_argumentInfos };
		}
	}

	// Register Construct functions
	int constructFunctionId = engine->RegisterGlobalFunction(
		"NativeObject@ __CreateNativeObject(const string &in className)",
		asFUNCTION(CreateNativeObject),
		asCALL_CDECL);
	GUARANTEE_OR_DIE(
		constructFunctionId >= 0,
		Stringf("Failed to register bridge function: __CreateNativeObject(const string &in className)"));

	std::vector<ClassInfo const*>   classes = ClassDatabase::GetRegisteredClasses();
	std::unordered_set<std::string> registeredFunctions;
	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr)
		{
			continue;
		}

		// If the class is a subclass of SystemBase
		// we will register its methods as global functions
		if (classInfo->m_parentClassName == SystemBase::GetStaticClassName())
		{
			for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
			{
				if (methodInfo == nullptr)
				{
					continue;
				}

				std::string scriptDeclaration =
					BuildBridgeFunctionDeclaration(*methodInfo, ScriptCallableKind::GlobalObject);
				if (registeredFunctions.find(scriptDeclaration) != registeredFunctions.end())
				{
					continue;
				}

				int functionId = engine->RegisterGlobalFunction(
					scriptDeclaration.c_str(),
					asFUNCTION(BridgeGlobalObjectGeneric),
					asCALL_GENERIC);
				GUARANTEE_OR_DIE(
					functionId >= 0,
					Stringf("Failed to register bridge function: %s", scriptDeclaration.c_str()));

				registeredFunctions.insert(scriptDeclaration);
				s_bridgeSignatures[functionId] = { methodInfo->m_returnType, methodInfo->m_argumentInfos };
			}
		}
		else
		{
			// Register Object methods
			for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
			{
				if (methodInfo == nullptr)
				{
					continue;
				}

				std::string scriptDeclaration = BuildBridgeFunctionDeclaration(*methodInfo, ScriptCallableKind::Object);
				if (registeredFunctions.find(scriptDeclaration) != registeredFunctions.end())
				{
					continue;
				}

				int functionId = engine->RegisterGlobalFunction(
					scriptDeclaration.c_str(),
					asFUNCTION(BridgeObjectGeneric),
					asCALL_GENERIC);
				GUARANTEE_OR_DIE(
					functionId >= 0,
					Stringf("Failed to register bridge function: %s", scriptDeclaration.c_str()));

				registeredFunctions.insert(scriptDeclaration);
				s_bridgeSignatures[functionId] = { methodInfo->m_returnType, methodInfo->m_argumentInfos };
			}
		}
	}
}
