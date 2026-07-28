#include "MingEngine/Scene/Resource/ShaderResourceFormat.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

namespace
{
std::string const kShaderExtension = ".hlsl";
}

std::vector<std::string> ShaderResourceLoader::GetSupportedExtensions() const { return { kShaderExtension }; }

Ref<Resource> ShaderResourceLoader::Load(VirtualPath const& virtualPath)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr || g_engine->m_renderer == nullptr)
	{
		return Ref<Resource>();
	}

	std::filesystem::path physicalPath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(virtualPath, physicalPath))
	{
		ERROR_AND_DIE(Stringf("Failed to resolve shader file \"%s\"", virtualPath.CStr()));
	}

	std::string shaderSource;
	if (!g_engine->m_fileSystem->ReadText(virtualPath, shaderSource))
	{
		ERROR_AND_DIE(Stringf("Failed to read shader file \"%s\"", virtualPath.CStr()));
	};

	Shader* shader =
		g_engine->m_renderer->CreateShader(virtualPath.GetString(), shaderSource, physicalPath.string());
	if (shader == nullptr)
	{
		ERROR_AND_DIE(Stringf("Failed to create shader from file \"%s\"", virtualPath.CStr()));
	}

	Ref<ShaderResource> shaderResource = CreateRef<ShaderResource>();
	shaderResource->SetVirtualPath(virtualPath);
	shaderResource->SetName(physicalPath.stem().string());
	shaderResource->SetShader(shader);

	return shaderResource;
}
