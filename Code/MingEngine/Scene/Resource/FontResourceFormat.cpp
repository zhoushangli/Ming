#include "MingEngine/Scene/Resource/FontResourceFormat.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"

#include <utility>

std::vector<std::string> FontResourceLoader::GetSupportedExtensions() const { return { ".ttf" }; }

Ref<Resource> FontResourceLoader::Load(VirtualPath const& path)
{
	if (g_engine == nullptr || g_engine->m_fileSystem == nullptr || g_engine->m_textServer == nullptr)
	{
		return {};
	}

	std::vector<uint8_t> bytes;
	if (!g_engine->m_fileSystem->ReadBinary(path, bytes))
	{
		return {};
	}

	RID const rid = g_engine->m_textServer->CreateFont(std::move(bytes));
	if (!rid.IsValid())
	{
		return {};
	}

	Ref<FontResource> resource = CreateRef<FontResource>();
	resource->SetVirtualPath(path);
	resource->SetName(path.GetFileName());
	resource->SetFontRID(rid);
	return resource;
}