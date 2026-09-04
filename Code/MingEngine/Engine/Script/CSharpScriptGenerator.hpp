#pragma once

#include "MingEngine/Core/Object/Variant.hpp"

#include <filesystem>
#include <map>
#include <string>

// CSharpTypeInfo fields are inserted into the following generated C# shape.
// ObjectPtr public types use ArgumentInfo.m_objectClassName instead of m_csType.
/*
public {RETURN_TYPE} Method({ARGUMENT_TYPE} arg1)
{
	{RETURN.m_csOutExpression}
		{OBJECT_RETURN_CAST}{CALL} = NativeCalls.MingCall_{RETURN.m_name}_{ARG.m_name}(
			methodBind, objectPtr, {ARG.m_csInExpression});
		{VALUE} in ARG.m_csInExpression = arg1
}

internal static {RETURN.m_callTypeOut} MingCall_{RETURN.m_name}_{ARG.m_name}(
	IntPtr methodBind, IntPtr objectPtr, {ARG.m_callTypeIn} arg1)
{
	{RETURN.m_disposableReturn ? "using " : ""}{RETURN.m_ptrCallType} ret
		{RETURN.m_disposableReturn || RETURN.m_defaultInitializeReturn ? " = default" : ""};

	{ARG.m_callIn}
		{VALUE} in ARG.m_callIn = arg1

	void** args = stackalloc void*[1] { {ARG.m_ptrCallArgument} };
		{VALUE} in ARG.m_ptrCallArgument = arg1

	NativeFuncs.MethodBindPtrCall(methodBind, objectPtr, args,
		{RETURN.m_callTypeOut != "void" ? "&ret" : "null"});

	{RETURN.m_callOut}
		{VALUE} in RETURN.m_callOut = ret
};
*/
struct CSharpTypeInfo
{
	std::string m_name;
	std::string m_csType;
	std::string m_callTypeIn;
	std::string m_callTypeOut;
	std::string m_ptrCallType;

	std::string m_csInExpression;
	std::string m_csOutExpression;
	std::string m_callIn;
	std::string m_ptrCallArgument;
	std::string m_callOut;

	bool m_disposableReturn        = false;
	bool m_defaultInitializeReturn = false;
};

class CSharpScriptGenerator
{
public:
	CSharpScriptGenerator();

	bool GenerateCSharpBindings(std::filesystem::path const& outputDirectory);

private:
	std::map<Variant::Type, CSharpTypeInfo> m_builtinTypes;
};
