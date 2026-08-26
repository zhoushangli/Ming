#pragma once

#include "MingEngine/Core/Object/Variant.hpp"

#include <filesystem>
#include <map>
#include <string>

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
