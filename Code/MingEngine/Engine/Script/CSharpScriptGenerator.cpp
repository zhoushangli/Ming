#include "MingEngine/Engine/Script/CSharpScriptGenerator.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"

#include <fstream>

namespace
{
constexpr char const* MingObjectTemplate = R"(namespace MingSharp
{
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
			return new MingObject(NativeCalls.CreateObject(className));
		}

		// Return the engine class name of this object.
		// e.g. node.GetClassName() -> "Node"
		public string GetClassName()
		{
			return NativeCalls.GetClassName(GetPtr(this));
		}
	}
}
)";

constexpr char const* RootClassTemplate = R"(using System.Diagnostics;

namespace MingSharp
{
	public partial class {CLASS_NAME} : MingObject
	{
{METHOD_BINDINGS}		internal {CLASS_NAME}(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
)";

constexpr char const* DerivedClassTemplate = R"(using System.Diagnostics;

namespace MingSharp
{
	public partial class {CLASS_NAME} : {PARENT_CLASS_NAME}
	{
{METHOD_BINDINGS}		internal {CLASS_NAME}(nint nativeHandle) : base(nativeHandle)
		{
		}
	}
}
)";

constexpr char const* MethodBindingTemplate = R"(		[DebuggerBrowsable(DebuggerBrowsableState.Never)]
		private static readonly IntPtr {METHOD_BIND_NAME} = NativeCalls.GetMethodBind("{CLASS_NAME}", "{METHOD_NAME}");

)";

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

	std::vector<ClassInfo const*> const classes = ClassDatabase::GetRegisteredClasses(true);
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
		}

		ReplaceAll(classSource, "{METHOD_BINDINGS}", methodBindings);
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

	return true;
}
