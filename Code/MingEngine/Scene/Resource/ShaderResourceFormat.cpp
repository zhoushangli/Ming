#include "MingEngine/Scene/Resource/ShaderResourceFormat.hpp"

#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

namespace
{
std::string const kShaderExtension = ".mshader";
}

std::vector<std::string> ShaderResourceLoader::GetSupportedExtensions() const
{
	return { kShaderExtension };
}

Ref<Resource> ShaderResourceLoader::Load(VirtualPath const& virtualPath)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr || g_engine->m_renderServer == nullptr)
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

	MShaderParseResult parsed;
	std::string diagnostic;
	if (!MShaderParser::Parse(shaderSource, physicalPath.string(), parsed, diagnostic))
	{
		ERR_PRINT(diagnostic);
		return Ref<Resource>();
	}

	std::string callbacks;
	if (!parsed.m_hasVertex)
	{
		callbacks += "\nvoid vertex(inout VertexContext v) {}\n";
	}
	if (!parsed.m_hasFragment)
	{
		callbacks += "\nvoid fragment(inout FragmentContext f) {}\n";
	}
	shaderSource = BuiltinShaders::BuildSurfaceShaderSource(
		parsed.m_source + "\n#line 1 \"Ming/DefaultCallbacks\"\n" + callbacks, physicalPath.string(), parsed.m_uniformSource);

	Shader* shader =
		g_engine->m_renderServer->CreateShader(virtualPath.GetString(), shaderSource, physicalPath.string());
	if (shader == nullptr)
	{
		ERR_PRINT("Failed to create MShader.");
		return Ref<Resource>();
	}

	Ref<ShaderResource> shaderResource = CreateRef<ShaderResource>();
	shaderResource->SetVirtualPath(virtualPath);
	shaderResource->SetName(physicalPath.stem().string());
	shaderResource->SetShader(shader);
	shaderResource->SetUniforms(std::move(parsed.m_uniforms));

	return shaderResource;
}
