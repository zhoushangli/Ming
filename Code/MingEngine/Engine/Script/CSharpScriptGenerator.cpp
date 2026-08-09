#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <algorithm>
#include <fstream>
#include <map>

namespace
{
constexpr char const* MingObjectTemplate = R"(namespace Ming;

public class MingObject
{
	internal IntPtr NativePtr;

	internal MingObject(IntPtr ptr)
	{
		NativePtr = ptr;
	}

	internal static IntPtr GetPtr(MingObject? obj)
	{
		if (obj == null)
		{
			return IntPtr.Zero;
		}

		ObjectDisposedException.ThrowIf(obj.NativePtr == IntPtr.Zero, obj);
		return obj.NativePtr;
	}

	// Create an engine object from the class database and return a C# wrapper of it.
	// e.g. MingObject.Create("Node")
	public static MingObject Create(string className)
	{
		return new MingObject(NativeFuncs.CreateObject(className));
	}

	// Return the engine class name of this object.
	// e.g. node.GetClassName() -> "Node"
	public string GetClassName()
	{
		return NativeFuncs.GetClassName(GetPtr(this));
	}
}
)";

constexpr char const* RootClassTemplate = R"(using System.Diagnostics;

namespace Ming;

public partial class {CLASS_NAME} : MingObject
{
{METHOD_BINDINGS}	internal {CLASS_NAME}(nint nativeHandle) : base(nativeHandle)
	{
	}

{CLASS_METHODS}}
)";

constexpr char const* DerivedClassTemplate = R"(using System.Diagnostics;

namespace Ming;

public partial class {CLASS_NAME} : {PARENT_CLASS_NAME}
{
{METHOD_BINDINGS}	internal {CLASS_NAME}(nint nativeHandle) : base(nativeHandle)
	{
	}

{CLASS_METHODS}}
)";

constexpr char const* MethodBindingTemplate = R"(	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
	private static readonly IntPtr {METHOD_BIND_NAME} = NativeFuncs.GetMethodBind("{CLASS_NAME}", "{METHOD_NAME}");

)";

constexpr char const* MethodTemplate = R"(	public {RETURN_TYPE} {METHOD_NAME}({METHOD_ARGUMENTS})
	{
		{METHOD_CALL}
	}

)";

constexpr char const* NativeCallsTemplate = R"(namespace Ming;

internal static unsafe class NativeCalls
{
{FUNCTIONS}}
)";

constexpr char const* NativeCallFunctionTemplate =
	R"(	internal static unsafe {RETURN_TYPE} {METHOD_NAME}(IntPtr methodBind, IntPtr objectPtr{ARGUMENTS})
	{
{RETURN_VALUE_DECLARATION}{ARGUMENTS_ARRAY_DECLARATION}		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, {ARGUMENTS_POINTER}, {RETURN_VALUE_POINTER});
{RETURN_VALUE_RETURN}	}

)";

struct NativeTypeInfo
{
	char const* m_name;
	char const* m_csharpType;
};

struct NativeCallInfo
{
	std::string              m_name;
	std::string              m_returnType;
	std::vector<std::string> m_argumentTypes;
};

char const* GetVariantTypeName(Variant::Type type)
{
	switch (type)
	{
	case Variant::Type::Empty:
		return "Empty";
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
	case Variant::Type::Color:
		return "Color";
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
		return "ObjectPtr";
	case Variant::Type::Any:
		return "Any";
	}

	return "Unknown";
}

bool TryGetNativeTypeInfo(Variant::Type type, bool isReturnType, NativeTypeInfo& outTypeInfo)
{
	switch (type)
	{
	case Variant::Type::Empty:
		if (isReturnType)
		{
			outTypeInfo = { "Void", "void" };
			return true;
		}
		return false;
	case Variant::Type::Bool:
		outTypeInfo = { "Bool", "MingBool" };
		return true;
	case Variant::Type::Int:
		outTypeInfo = { "Int", "int" };
		return true;
	case Variant::Type::Float:
		outTypeInfo = { "Float", "float" };
		return true;
	default:
		return false;
	}
}

bool TryCreateNativeCall(MethodInfo const& methodInfo, NativeCallInfo& outNativeCall, std::string& outReason)
{
	NativeTypeInfo returnTypeInfo;
	if (!TryGetNativeTypeInfo(methodInfo.m_returnInfo.m_type, true, returnTypeInfo))
	{
		outReason = std::string("unsupported return type ") + GetVariantTypeName(methodInfo.m_returnInfo.m_type);
		return false;
	}

	outNativeCall.m_name       = std::string("MingCall_") + returnTypeInfo.m_name;
	outNativeCall.m_returnType = returnTypeInfo.m_csharpType;
	outNativeCall.m_argumentTypes.clear();
	outNativeCall.m_argumentTypes.reserve(methodInfo.m_argumentInfos.size());

	for (ArgumentInfo const& argumentInfo : methodInfo.m_argumentInfos)
	{
		NativeTypeInfo argumentTypeInfo;
		if (!TryGetNativeTypeInfo(argumentInfo.m_type, false, argumentTypeInfo))
		{
			outReason = std::string("unsupported argument type ") + GetVariantTypeName(argumentInfo.m_type);
			return false;
		}

		outNativeCall.m_name += "_";
		outNativeCall.m_name += argumentTypeInfo.m_name;
		outNativeCall.m_argumentTypes.emplace_back(argumentTypeInfo.m_csharpType);
	}

	return true;
}

void ReplaceAll(std::string& source, std::string const& placeholder, std::string const& value);

char const* GetMethodCSharpType(Variant::Type type)
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
	default:
		return nullptr;
	}
}

std::string GenerateClassMethod(MethodInfo const& methodInfo, NativeCallInfo const& nativeCall)
{
	std::string methodArguments;
	std::string callArguments;
	for (size_t argumentIndex = 0; argumentIndex < methodInfo.m_argumentInfos.size(); ++argumentIndex)
	{
		ArgumentInfo const& argumentInfo = methodInfo.m_argumentInfos[argumentIndex];
		std::string const   argumentName = "arg" + std::to_string(argumentIndex + 1);
		if (!methodArguments.empty())
		{
			methodArguments += ", ";
		}
		methodArguments += GetMethodCSharpType(argumentInfo.m_type);
		methodArguments += " ";
		methodArguments += argumentName;

		callArguments += ", ";
		callArguments += argumentName;
		if (argumentInfo.m_type == Variant::Type::Bool)
		{
			callArguments += ".ToMingBool()";
		}
	}

	std::string methodCall;
	if (methodInfo.m_returnInfo.m_type != Variant::Type::Empty)
	{
		methodCall = "return ";
	}
	methodCall +=
		"NativeCalls." + nativeCall.m_name + "(" + methodInfo.m_name + "MethodBind, GetPtr(this)" + callArguments + ")";
	if (methodInfo.m_returnInfo.m_type == Variant::Type::Bool)
	{
		methodCall += ".ToBool()";
	}
	methodCall += ";";

	std::string source = MethodTemplate;
	ReplaceAll(source, "{RETURN_TYPE}", GetMethodCSharpType(methodInfo.m_returnInfo.m_type));
	ReplaceAll(source, "{METHOD_NAME}", methodInfo.m_name);
	ReplaceAll(source, "{METHOD_ARGUMENTS}", methodArguments);
	ReplaceAll(source, "{METHOD_CALL}", methodCall);
	return source;
}

std::string GenerateNativeCallFunction(NativeCallInfo const& nativeCall)
{
	std::string arguments;
	std::string argumentPointers;

	for (size_t argumentIndex = 0; argumentIndex < nativeCall.m_argumentTypes.size(); ++argumentIndex)
	{
		std::string const argumentName = "arg" + std::to_string(argumentIndex + 1);
		arguments += ", " + nativeCall.m_argumentTypes[argumentIndex] + " " + argumentName;
		if (!argumentPointers.empty())
		{
			argumentPointers += ", ";
		}
		argumentPointers += "&" + argumentName;
	}

	bool const  hasReturnValue         = nativeCall.m_returnType != "void";
	std::string returnValueDeclaration = hasReturnValue ? "\t\t" + nativeCall.m_returnType + " ret;\n\n" : "";
	std::string argumentsArrayDeclaration;
	if (!nativeCall.m_argumentTypes.empty())
	{
		argumentsArrayDeclaration = "\t\tvoid** args = stackalloc void*["
									+ std::to_string(nativeCall.m_argumentTypes.size()) + "] { " + argumentPointers
									+ " };\n\n";
	}

	std::string source = NativeCallFunctionTemplate;
	ReplaceAll(source, "{RETURN_TYPE}", nativeCall.m_returnType);
	ReplaceAll(source, "{METHOD_NAME}", nativeCall.m_name);
	ReplaceAll(source, "{ARGUMENTS}", arguments);
	ReplaceAll(source, "{RETURN_VALUE_DECLARATION}", returnValueDeclaration);
	ReplaceAll(source, "{ARGUMENTS_ARRAY_DECLARATION}", argumentsArrayDeclaration);
	ReplaceAll(source, "{ARGUMENTS_POINTER}", nativeCall.m_argumentTypes.empty() ? "null" : "args");
	ReplaceAll(source, "{RETURN_VALUE_POINTER}", hasReturnValue ? "&ret" : "null");
	ReplaceAll(source, "{RETURN_VALUE_RETURN}", hasReturnValue ? "\n\t\treturn ret;\n" : "");
	return source;
}

void ReplaceAll(std::string& source, std::string const& placeholder, std::string const& value)
{
	size_t position = 0;
	while ((position = source.find(placeholder, position)) != std::string::npos)
	{
		source.replace(position, placeholder.length(), value);
		position += value.length();
	}
}
} // namespace

bool CSharpScriptGenerator::GenerateCSharpBindings(std::filesystem::path const& outputDirectory)
{
	if (outputDirectory.empty())
	{
		return false;
	}

	std::filesystem::path const generatedDirectory = outputDirectory / "Generated";
	std::error_code             errorCode;
	std::filesystem::create_directories(generatedDirectory, errorCode);
	if (errorCode)
	{
		return false;
	}

	for (std::filesystem::directory_iterator iter(generatedDirectory, errorCode), end; iter != end;
		 iter.increment(errorCode))
	{
		if (errorCode)
		{
			return false;
		}

		std::filesystem::directory_entry const& entry = *iter;
		if (entry.path().extension() != ".cs" || !entry.is_regular_file(errorCode))
		{
			if (errorCode)
			{
				return false;
			}
			continue;
		}

		std::filesystem::remove(entry.path(), errorCode);
		if (errorCode)
		{
			return false;
		}
	}
	if (errorCode)
	{
		return false;
	}

	// Generate the root C# wrapper class that all engine classes inherit from.
	std::string const           mingObjectSource   = MingObjectTemplate;
	std::filesystem::path const mingObjectFilePath = generatedDirectory / "MingObject.cs";
	std::ofstream               mingObjectFile(mingObjectFilePath, std::ios::binary | std::ios::trunc);
	if (!mingObjectFile.is_open())
	{
		return false;
	}

	mingObjectFile.write(mingObjectSource.data(), static_cast<std::streamsize>(mingObjectSource.size()));
	mingObjectFile.close();
	if (mingObjectFile.fail())
	{
		return false;
	}

	std::vector<ClassInfo const*> const   classes = ClassDatabase::GetRegisteredClasses(true);
	std::map<std::string, NativeCallInfo> nativeCalls;
	std::vector<std::string>              skippedNativeCalls;
	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr || classInfo->m_apiType != ApiType::Runtime)
		{
			continue;
		}

		std::string classSource;
		if (classInfo->m_parentClassName.empty())
		{
			classSource = RootClassTemplate;
		}
		else
		{
			classSource = DerivedClassTemplate;
			std::string const parentClassName =
				classInfo->m_parentClassName == "Object" ? "MingObject" : classInfo->m_parentClassName;
			ReplaceAll(classSource, "{PARENT_CLASS_NAME}", parentClassName);
		}

		std::string methodBindings;
		std::string classMethods;
		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo == nullptr)
			{
				continue;
			}

			std::string methodBinding = MethodBindingTemplate;
			ReplaceAll(methodBinding, "{METHOD_BIND_NAME}", methodInfo->m_name + "MethodBind");
			ReplaceAll(methodBinding, "{CLASS_NAME}", classInfo->m_className);
			ReplaceAll(methodBinding, "{METHOD_NAME}", methodInfo->m_name);
			methodBindings += methodBinding;

			NativeCallInfo nativeCall;
			std::string    skipReason;
			if (TryCreateNativeCall(*methodInfo, nativeCall, skipReason))
			{
				classMethods += GenerateClassMethod(*methodInfo, nativeCall);
				std::string const nativeCallName = nativeCall.m_name;
				nativeCalls.emplace(nativeCallName, std::move(nativeCall));
			}
			else
			{
				skippedNativeCalls.emplace_back(classInfo->m_className + "." + methodInfo->m_name + ": " + skipReason);
			}
		}

		ReplaceAll(classSource, "{METHOD_BINDINGS}", methodBindings);
		ReplaceAll(classSource, "{CLASS_METHODS}", classMethods);
		ReplaceAll(classSource, "{CLASS_NAME}", classInfo->m_className);

		std::filesystem::path const classFilePath = generatedDirectory / (classInfo->m_className + ".cs");
		std::ofstream               classFile(classFilePath, std::ios::binary | std::ios::trunc);
		if (!classFile.is_open())
		{
			return false;
		}

		classFile.write(classSource.data(), static_cast<std::streamsize>(classSource.size()));
		classFile.close();
		if (classFile.fail())
		{
			return false;
		}
	}

	std::string nativeCallFunctions;
	for (auto const& [nativeCallName, nativeCall] : nativeCalls)
	{
		(void)nativeCallName;
		nativeCallFunctions += GenerateNativeCallFunction(nativeCall);
	}

	std::string nativeCallsSource = NativeCallsTemplate;
	ReplaceAll(nativeCallsSource, "{FUNCTIONS}", nativeCallFunctions);

	std::filesystem::path const nativeCallsFilePath = generatedDirectory / "NativeCalls.cs";
	std::ofstream               nativeCallsFile(nativeCallsFilePath, std::ios::binary | std::ios::trunc);
	if (!nativeCallsFile.is_open())
	{
		return false;
	}

	nativeCallsFile.write(nativeCallsSource.data(), static_cast<std::streamsize>(nativeCallsSource.size()));
	nativeCallsFile.close();
	if (nativeCallsFile.fail())
	{
		return false;
	}

	std::sort(skippedNativeCalls.begin(), skippedNativeCalls.end());
	if (!skippedNativeCalls.empty())
	{
		DebuggerPrintf("Skipped unsupported NativeCalls signatures:\n");
		for (std::string const& skippedNativeCall : skippedNativeCalls)
		{
			DebuggerPrintf("- %s\n", skippedNativeCall.c_str());
		}
	}

	return true;
}
