#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <utility>

namespace
{
constexpr char const* ClassTemplate = R"(using System.Diagnostics;

namespace Ming;

public partial class {CLASS_NAME} : {PARENT_CLASS_NAME}
{
    private static readonly Type CachedType = typeof({CLASS_NAME});
    private const string NativeName = "{CLASS_NAME}";

	[DebuggerBrowsable(DebuggerBrowsableState.Never)]
    private unsafe static readonly delegate* unmanaged<IntPtr> NativeCtor = GetConstrcutor(NativeName);

{METHOD_BINDINGS}	public unsafe {CLASS_NAME}() : base(false)
    {
        ConstructAndInitialize(NativeCtor, NativeName, CachedType);
    }

    public unsafe {CLASS_NAME}(nint nativePtr) : base(false)
    {
        NativePtr = nativePtr;
        ConstructAndInitialize(NativeCtor, NativeName, CachedType);
    }

    public unsafe {CLASS_NAME}(bool initialize) : base(initialize)
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
	R"(	internal static {RETURN_TYPE} {METHOD_NAME}(IntPtr methodBind, IntPtr objectPtr{ARGUMENTS})
	{
{RETURN_VALUE_DECLARATION}{ARGUMENT_CONVERSIONS}{ARGUMENTS_ARRAY_DECLARATION}		NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, {ARGUMENTS_POINTER}, {RETURN_VALUE_POINTER});
{RETURN_VALUE_RETURN}	}

)";

struct NativeCallInfo
{
	std::string                 m_name;
	CSharpTypeInfo              m_returnType;
	std::vector<CSharpTypeInfo> m_argumentTypes;
};

using NativeCallMap       = std::map<std::string, NativeCallInfo>;
using MethodNativeCallMap = std::map<MethodInfo const*, std::string>;
using CSharpTypeMap       = std::map<Variant::Type, CSharpTypeInfo>;

void ReplaceAll(std::string& source, std::string const& placeholder, std::string const& value)
{
	size_t position = 0;
	while ((position = source.find(placeholder, position)) != std::string::npos)
	{
		source.replace(position, placeholder.length(), value);
		position += value.length();
	}
}

std::string FormatTypeExpression(
	std::string expression, std::string const& value = std::string(), std::string const& call = std::string())
{
	ReplaceAll(expression, "{VALUE}", value);
	ReplaceAll(expression, "{CALL}", call);
	return expression;
}

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
	case Variant::Type::Vector2:
		return "Vector2";
	case Variant::Type::Vector3:
		return "Vector3";
	case Variant::Type::Vector4:
		return "Vector4";
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

bool TryCreateNativeCall(
	MethodInfo const&    methodInfo,
	CSharpTypeMap const& builtinTypes,
	NativeCallInfo&      outNativeCall,
	std::string&         outReason)
{
	auto const returnTypeIter = builtinTypes.find(methodInfo.m_returnInfo.m_type);
	if (returnTypeIter == builtinTypes.end())
	{
		outReason = std::string("unsupported return type ") + GetVariantTypeName(methodInfo.m_returnInfo.m_type);
		return false;
	}

	outNativeCall.m_name       = std::string("MingCall_") + returnTypeIter->second.m_name;
	outNativeCall.m_returnType = returnTypeIter->second;
	outNativeCall.m_argumentTypes.clear();
	outNativeCall.m_argumentTypes.reserve(methodInfo.m_argumentInfos.size());

	for (ArgumentInfo const& argumentInfo : methodInfo.m_argumentInfos)
	{
		auto const argumentTypeIter = builtinTypes.find(argumentInfo.m_type);
		if (argumentInfo.m_type == Variant::Type::Empty || argumentTypeIter == builtinTypes.end())
		{
			outReason = std::string("unsupported argument type ") + GetVariantTypeName(argumentInfo.m_type);
			return false;
		}

		outNativeCall.m_name += "_";
		outNativeCall.m_name += argumentTypeIter->second.m_name;
		outNativeCall.m_argumentTypes.emplace_back(argumentTypeIter->second);
	}

	return true;
}

void CollectBindings(
	std::vector<ClassInfo const*> const& classes,
	CSharpTypeMap const&                 builtinTypes,
	NativeCallMap&                       outNativeCalls,
	MethodNativeCallMap&                 outMethodNativeCalls,
	std::vector<std::string>&            outSkippedNativeCalls)
{
	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr || classInfo->m_apiType != ApiType::Runtime)
		{
			continue;
		}

		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo == nullptr)
			{
				continue;
			}

			NativeCallInfo nativeCall;
			std::string    skipReason;
			if (!TryCreateNativeCall(*methodInfo, builtinTypes, nativeCall, skipReason))
			{
				outSkippedNativeCalls.emplace_back(
					classInfo->m_className + "." + methodInfo->m_name + ": " + skipReason);
				continue;
			}

			std::string const nativeCallName = nativeCall.m_name;
			outNativeCalls.emplace(nativeCallName, std::move(nativeCall));
			outMethodNativeCalls.emplace(methodInfo.get(), nativeCallName);
		}
	}
}

std::string GenerateClassMethod(MethodInfo const& methodInfo, NativeCallInfo const& nativeCall)
{
	std::string methodArguments;
	std::string callArguments;
	for (size_t argumentIndex = 0; argumentIndex < nativeCall.m_argumentTypes.size(); ++argumentIndex)
	{
		CSharpTypeInfo const& argumentTypeInfo = nativeCall.m_argumentTypes[argumentIndex];
		std::string const     argumentName     = "arg" + std::to_string(argumentIndex + 1);
		if (!methodArguments.empty())
		{
			methodArguments += ", ";
		}
		methodArguments += argumentTypeInfo.m_csType + " " + argumentName;

		callArguments += ", ";
		callArguments += FormatTypeExpression(argumentTypeInfo.m_csInExpression, argumentName);
	}

	std::string const nativeCallExpression =
		"NativeCalls." + nativeCall.m_name + "(" + methodInfo.m_name + "MethodBind, GetPtr(this)" + callArguments + ")";
	std::string methodCall;
	if (nativeCall.m_returnType.m_csType == "void")
	{
		methodCall = nativeCallExpression + ";";
	}
	else
	{
		methodCall =
			FormatTypeExpression(nativeCall.m_returnType.m_csOutExpression, std::string(), nativeCallExpression);
	}

	std::string source = MethodTemplate;
	ReplaceAll(source, "{RETURN_TYPE}", nativeCall.m_returnType.m_csType);
	ReplaceAll(source, "{METHOD_NAME}", methodInfo.m_name);
	ReplaceAll(source, "{METHOD_ARGUMENTS}", methodArguments);
	ReplaceAll(source, "{METHOD_CALL}", methodCall);
	return source;
}

std::string GenerateNativeCallFunction(NativeCallInfo const& nativeCall)
{
	std::string arguments;
	std::string argumentConversions;
	std::string argumentPointers;

	for (size_t argumentIndex = 0; argumentIndex < nativeCall.m_argumentTypes.size(); ++argumentIndex)
	{
		CSharpTypeInfo const& argumentTypeInfo = nativeCall.m_argumentTypes[argumentIndex];
		std::string const     argumentName     = "arg" + std::to_string(argumentIndex + 1);
		arguments += ", " + argumentTypeInfo.m_callTypeIn + " " + argumentName;

		if (!argumentTypeInfo.m_callIn.empty())
		{
			argumentConversions += "\t\t" + FormatTypeExpression(argumentTypeInfo.m_callIn, argumentName) + "\n\n";
		}

		if (!argumentPointers.empty())
		{
			argumentPointers += ", ";
		}
		argumentPointers += FormatTypeExpression(argumentTypeInfo.m_ptrCallArgument, argumentName);
	}

	bool const  hasReturnValue = nativeCall.m_returnType.m_callTypeOut != "void";
	std::string returnValueDeclaration;
	if (hasReturnValue)
	{
		returnValueDeclaration = "\t\t";
		if (nativeCall.m_returnType.m_disposableReturn)
		{
			returnValueDeclaration += "using ";
		}
		returnValueDeclaration += nativeCall.m_returnType.m_ptrCallType + " ret";
		if (nativeCall.m_returnType.m_disposableReturn || nativeCall.m_returnType.m_defaultInitializeReturn)
		{
			returnValueDeclaration += " = default";
		}
		returnValueDeclaration += ";\n\n";
	}

	std::string argumentsArrayDeclaration;
	if (!nativeCall.m_argumentTypes.empty())
	{
		argumentsArrayDeclaration = "\t\tvoid** args = stackalloc void*["
									+ std::to_string(nativeCall.m_argumentTypes.size()) + "] { " + argumentPointers
									+ " };\n\n";
	}

	std::string returnValueReturn;
	if (hasReturnValue)
	{
		returnValueReturn = "\n\t\t" + FormatTypeExpression(nativeCall.m_returnType.m_callOut, "ret") + "\n";
	}

	std::string source = NativeCallFunctionTemplate;
	ReplaceAll(source, "{RETURN_TYPE}", nativeCall.m_returnType.m_callTypeOut);
	ReplaceAll(source, "{METHOD_NAME}", nativeCall.m_name);
	ReplaceAll(source, "{ARGUMENTS}", arguments);
	ReplaceAll(source, "{RETURN_VALUE_DECLARATION}", returnValueDeclaration);
	ReplaceAll(source, "{ARGUMENT_CONVERSIONS}", argumentConversions);
	ReplaceAll(source, "{ARGUMENTS_ARRAY_DECLARATION}", argumentsArrayDeclaration);
	ReplaceAll(source, "{ARGUMENTS_POINTER}", nativeCall.m_argumentTypes.empty() ? "null" : "args");
	ReplaceAll(source, "{RETURN_VALUE_POINTER}", hasReturnValue ? "&ret" : "null");
	ReplaceAll(source, "{RETURN_VALUE_RETURN}", returnValueReturn);
	return source;
}

bool WriteTextFile(std::filesystem::path const& filePath, std::string const& contents)
{
	std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
	if (!file.is_open())
	{
		return false;
	}

	file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
	file.close();
	return !file.fail();
}

bool GenerateNativeCalls(std::filesystem::path const& outputDirectory, NativeCallMap const& nativeCalls)
{
	std::string nativeCallFunctions;
	for (auto const& [nativeCallName, nativeCall] : nativeCalls)
	{
		(void)nativeCallName;
		nativeCallFunctions += GenerateNativeCallFunction(nativeCall);
	}

	std::string nativeCallsSource = NativeCallsTemplate;
	ReplaceAll(nativeCallsSource, "{FUNCTIONS}", nativeCallFunctions);
	return WriteTextFile(outputDirectory / "NativeCalls.cs", nativeCallsSource);
}

bool GenerateClassBindings(
	std::filesystem::path const&         outputDirectory,
	std::vector<ClassInfo const*> const& classes,
	NativeCallMap const&                 nativeCalls,
	MethodNativeCallMap const&           methodNativeCalls)
{
	for (ClassInfo const* classInfo : classes)
	{
		if (classInfo == nullptr || classInfo->m_apiType != ApiType::Runtime || classInfo->m_className == "Object")
		{
			continue;
		}

		if (classInfo->m_parentClassName.empty())
		{
			return false;
		}

		std::string       classSource = ClassTemplate;
		std::string const parentClassName =
			classInfo->m_parentClassName == "Object" ? "MingObject" : classInfo->m_parentClassName;
		ReplaceAll(classSource, "{PARENT_CLASS_NAME}", parentClassName);

		std::string methodBindings;
		std::string classMethods;
		for (std::unique_ptr<MethodInfo> const& methodInfo : classInfo->m_methods)
		{
			if (methodInfo == nullptr)
			{
				continue;
			}

			auto const methodCallIter = methodNativeCalls.find(methodInfo.get());
			if (methodCallIter == methodNativeCalls.end())
			{
				continue;
			}

			auto const nativeCallIter = nativeCalls.find(methodCallIter->second);
			if (nativeCallIter == nativeCalls.end())
			{
				return false;
			}

			std::string methodBinding = MethodBindingTemplate;
			ReplaceAll(methodBinding, "{METHOD_BIND_NAME}", methodInfo->m_name + "MethodBind");
			ReplaceAll(methodBinding, "{CLASS_NAME}", classInfo->m_className);
			ReplaceAll(methodBinding, "{METHOD_NAME}", methodInfo->m_name);
			methodBindings += methodBinding;
			classMethods += GenerateClassMethod(*methodInfo, nativeCallIter->second);
		}

		ReplaceAll(classSource, "{METHOD_BINDINGS}", methodBindings);
		ReplaceAll(classSource, "{CLASS_METHODS}", classMethods);
		ReplaceAll(classSource, "{CLASS_NAME}", classInfo->m_className);

		if (!WriteTextFile(outputDirectory / (classInfo->m_className + ".cs"), classSource))
		{
			return false;
		}
	}

	return true;
}
} // namespace

CSharpScriptGenerator::CSharpScriptGenerator()
{
	CSharpTypeInfo typeInfo;

	{
		typeInfo                             = {};
		typeInfo.m_name                      = "Void";
		typeInfo.m_csType                    = "void";
		typeInfo.m_callTypeIn                = "void";
		typeInfo.m_callTypeOut               = "void";
		typeInfo.m_ptrCallType               = "void";
		typeInfo.m_csInExpression            = "{VALUE}";
		typeInfo.m_csOutExpression           = "return {CALL};";
		typeInfo.m_ptrCallArgument           = "&{VALUE}";
		typeInfo.m_callOut                   = "return {VALUE};";
		m_builtinTypes[Variant::Type::Empty] = typeInfo;
	}

	{
		typeInfo                            = {};
		typeInfo.m_name                     = "Bool";
		typeInfo.m_csType                   = "bool";
		typeInfo.m_callTypeIn               = "MingBool";
		typeInfo.m_callTypeOut              = "MingBool";
		typeInfo.m_ptrCallType              = "MingBool";
		typeInfo.m_csInExpression           = "{VALUE}.ToMingBool()";
		typeInfo.m_csOutExpression          = "return {CALL}.ToBool();";
		typeInfo.m_ptrCallArgument          = "&{VALUE}";
		typeInfo.m_callOut                  = "return {VALUE};";
		m_builtinTypes[Variant::Type::Bool] = typeInfo;
	}

	{
		typeInfo                           = {};
		typeInfo.m_name                    = "Int";
		typeInfo.m_csType                  = "int";
		typeInfo.m_callTypeIn              = "int";
		typeInfo.m_callTypeOut             = "int";
		typeInfo.m_ptrCallType             = "int";
		typeInfo.m_csInExpression          = "{VALUE}";
		typeInfo.m_csOutExpression         = "return {CALL};";
		typeInfo.m_ptrCallArgument         = "&{VALUE}";
		typeInfo.m_callOut                 = "return {VALUE};";
		m_builtinTypes[Variant::Type::Int] = typeInfo;
	}

	{
		typeInfo                             = {};
		typeInfo.m_name                      = "Float";
		typeInfo.m_csType                    = "float";
		typeInfo.m_callTypeIn                = "float";
		typeInfo.m_callTypeOut               = "float";
		typeInfo.m_ptrCallType               = "float";
		typeInfo.m_csInExpression            = "{VALUE}";
		typeInfo.m_csOutExpression           = "return {CALL};";
		typeInfo.m_ptrCallArgument           = "&{VALUE}";
		typeInfo.m_callOut                   = "return {VALUE};";
		m_builtinTypes[Variant::Type::Float] = typeInfo;
	}

	{
		typeInfo                    = {};
		typeInfo.m_name             = "String";
		typeInfo.m_csType           = "string";
		typeInfo.m_callTypeIn       = "string";
		typeInfo.m_callTypeOut      = "string";
		typeInfo.m_ptrCallType      = "MingString";
		typeInfo.m_csInExpression   = "{VALUE}";
		typeInfo.m_csOutExpression  = "return {CALL};";
		typeInfo.m_callIn           = "using MingString {VALUE}Native = Marshaling.ConvertStringToNative({VALUE});";
		typeInfo.m_ptrCallArgument  = "&{VALUE}Native";
		typeInfo.m_callOut          = "return Marshaling.ConvertStringToManaged({VALUE});";
		typeInfo.m_disposableReturn = true;
		typeInfo.m_defaultInitializeReturn    = true;
		m_builtinTypes[Variant::Type::String] = typeInfo;
	}

	{
		typeInfo                               = {};
		typeInfo.m_name                        = "Vector2";
		typeInfo.m_csType                      = "Vector2";
		typeInfo.m_callTypeIn                  = "Vector2";
		typeInfo.m_callTypeOut                 = "Vector2";
		typeInfo.m_ptrCallType                 = "Vector2";
		typeInfo.m_csInExpression              = "{VALUE}";
		typeInfo.m_csOutExpression             = "return {CALL};";
		typeInfo.m_ptrCallArgument             = "&{VALUE}";
		typeInfo.m_callOut                     = "return {VALUE};";
		m_builtinTypes[Variant::Type::Vector2] = typeInfo;
	}

	{
		typeInfo                               = {};
		typeInfo.m_name                        = "Vector3";
		typeInfo.m_csType                      = "Vector3";
		typeInfo.m_callTypeIn                  = "Vector3";
		typeInfo.m_callTypeOut                 = "Vector3";
		typeInfo.m_ptrCallType                 = "Vector3";
		typeInfo.m_csInExpression              = "{VALUE}";
		typeInfo.m_csOutExpression             = "return {CALL};";
		typeInfo.m_ptrCallArgument             = "&{VALUE}";
		typeInfo.m_callOut                     = "return {VALUE};";
		m_builtinTypes[Variant::Type::Vector3] = typeInfo;
	}

	{
		typeInfo                               = {};
		typeInfo.m_name                        = "Vector4";
		typeInfo.m_csType                      = "Vector4";
		typeInfo.m_callTypeIn                  = "Vector4";
		typeInfo.m_callTypeOut                 = "Vector4";
		typeInfo.m_ptrCallType                 = "Vector4";
		typeInfo.m_csInExpression              = "{VALUE}";
		typeInfo.m_csOutExpression             = "return {CALL};";
		typeInfo.m_ptrCallArgument             = "&{VALUE}";
		typeInfo.m_callOut                     = "return {VALUE};";
		m_builtinTypes[Variant::Type::Vector4] = typeInfo;
	}

	{
		typeInfo                             = {};
		typeInfo.m_name                      = "Color";
		typeInfo.m_csType                    = "Color";
		typeInfo.m_callTypeIn                = "Color";
		typeInfo.m_callTypeOut               = "Color";
		typeInfo.m_ptrCallType               = "Color";
		typeInfo.m_csInExpression            = "{VALUE}";
		typeInfo.m_csOutExpression           = "return {CALL};";
		typeInfo.m_ptrCallArgument           = "&{VALUE}";
		typeInfo.m_callOut                   = "return {VALUE};";
		m_builtinTypes[Variant::Type::Color] = typeInfo;
	}

	{
		typeInfo                                   = {};
		typeInfo.m_name                            = "EulerAngles";
		typeInfo.m_csType                          = "EulerAngles";
		typeInfo.m_callTypeIn                      = "EulerAngles";
		typeInfo.m_callTypeOut                     = "EulerAngles";
		typeInfo.m_ptrCallType                     = "EulerAngles";
		typeInfo.m_csInExpression                  = "{VALUE}";
		typeInfo.m_csOutExpression                 = "return {CALL};";
		typeInfo.m_ptrCallArgument                 = "&{VALUE}";
		typeInfo.m_callOut                         = "return {VALUE};";
		m_builtinTypes[Variant::Type::EulerAngles] = typeInfo;
	}

	{
		typeInfo                                 = {};
		typeInfo.m_name                          = "Matrix4x4";
		typeInfo.m_csType                        = "Matrix4x4";
		typeInfo.m_callTypeIn                    = "Matrix4x4";
		typeInfo.m_callTypeOut                   = "Matrix4x4";
		typeInfo.m_ptrCallType                   = "Matrix4x4";
		typeInfo.m_csInExpression                = "{VALUE}";
		typeInfo.m_csOutExpression               = "return {CALL};";
		typeInfo.m_ptrCallArgument               = "&{VALUE}";
		typeInfo.m_callOut                       = "return {VALUE};";
		m_builtinTypes[Variant::Type::Matrix4x4] = typeInfo;
	}
}

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

	std::vector<ClassInfo const*> const classes = ClassDatabase::GetRegisteredClasses(true);
	NativeCallMap                       nativeCalls;
	MethodNativeCallMap                 methodNativeCalls;
	std::vector<std::string>            skippedNativeCalls;
	CollectBindings(classes, m_builtinTypes, nativeCalls, methodNativeCalls, skippedNativeCalls);

	std::sort(skippedNativeCalls.begin(), skippedNativeCalls.end());
	if (!skippedNativeCalls.empty())
	{
		DebuggerPrintf("Skipped unsupported NativeCalls signatures:\n");
		for (std::string const& skippedNativeCall : skippedNativeCalls)
		{
			DebuggerPrintf("- %s\n", skippedNativeCall.c_str());
		}
	}

	if (!GenerateNativeCalls(generatedDirectory, nativeCalls))
	{
		ERROR_AND_DIE("Failed to generate C# NativeCalls.");
	}

	if (!GenerateClassBindings(generatedDirectory, classes, nativeCalls, methodNativeCalls))
	{
		ERROR_AND_DIE("Failed to generate C# Class Bindings.");
	}

	return true;
}
