#include "MingEngine/Engine/Script/ScriptResourceIdentity.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"

#include <cctype>

bool ScriptResourceIdentity::Create(VirtualPath const& path, ScriptResourceIdentity& outIdentity)
{
	std::string const& relativePath      = path.GetRelativePath();
	std::string        expectedExtension = ".as";

	if (relativePath.size() <= expectedExtension.size())
	{
		DebuggerPrintf(
			"Error: Script file '%s' is too short to contain a valid class name and the expected '%s' extension.\n",
			relativePath.c_str(),
			expectedExtension.c_str());
		return false;
	}

	if (relativePath.substr(relativePath.size() - 3) != expectedExtension)
	{
		DebuggerPrintf(
			"Error: Script file '%s' does not have the expected '%s' extension.\n",
			relativePath.c_str(),
			expectedExtension.c_str());
		return false;
	}

	size_t const fileNameBegin = relativePath.find_last_of('/');
	size_t const fileNameEnd   = relativePath.size() - 3; // Exclude ".as" extension
	std::string  className     = relativePath.substr(fileNameBegin + 1, fileNameEnd - fileNameBegin - 1);

	if (className.empty())
	{
		DebuggerPrintf("Error: Script file '%s' does not contain a valid class name.\n", relativePath.c_str());
		return false;
	}

	unsigned char first = static_cast<unsigned char>(className[0]);

	// The first character of a class name must be a letter or an underscore
	// Player  --> OK
	// _player --> OK
	// 3Player --> Illegal
	// -Test   --> Illegal
	if (!std::isalpha(first) && className[0] != '_')
	{
		return false;
	}

	// All characters in the class name must be letters, digits, or underscores
	for (char character : className)
	{
		unsigned char value = static_cast<unsigned char>(character);

		if (!std::isalnum(value) && character != '_')
		{
			return false;
		}
	}

	outIdentity.m_path      = path;
	outIdentity.m_className = className;

	return true;
}

VirtualPath const& ScriptResourceIdentity::GetPath() const { return m_path; }

std::string const& ScriptResourceIdentity::GetClassName() const { return m_className; }
