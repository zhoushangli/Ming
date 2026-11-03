#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/BuiltinIcons.hpp"
#include "MingEngine/Editor/BuiltinBrand.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"

std::map<std::string, Ref<TextureResource>> EditorIcons::m_icons;

Ref<TextureResource> EditorIcons::GetIconTexture(std::string const& name, std::string const& fallback)
{
	auto found = m_icons.find(name);
	if (found != m_icons.end())
	{
		return found->second;
	}

	BuiltinIcons::IconData const bytes = BuiltinIcons::Find(name);
	if (!bytes)
	{
		ClassInfo const* info = ClassDatabase::GetClassInfo(name);
		if (info != nullptr && !info->m_parentClassName.empty())
		{
			return GetIconTexture(info->m_parentClassName, fallback);
		}

		return fallback.empty() ? Ref<TextureResource>() : GetIconTexture(fallback);
	}

	auto image = CreateRef<Image>();
	if (!image->LoadFromMemory(std::vector<uint8_t>(bytes.m_data, bytes.m_data + bytes.m_size)))
	{
		return {};
	}

	auto texture           = CreateRef<TextureResource>();
	texture->m_image       = image;
	texture->m_samplerMode = SamplerMode::BILINEAR_CLAMP;
	if (!texture->InitGPUResources())
	{
		return {};
	}

	m_icons[name] = texture;
	return texture;
}

Color EditorIcons::GetIconColor(std::string const& name, Control const* source)
{
	std::string resolved = name;
	while (!BuiltinIcons::Find(resolved))
	{
		ClassInfo const* info = ClassDatabase::GetClassInfo(resolved);
		if (info == nullptr || info->m_parentClassName.empty())
		{
			break;
		}

		resolved = info->m_parentClassName;
	}

	return source->GetThemeColor(std::string(BuiltinIcons::GetTone(resolved)), "EditorIcons");
}

Ref<TextureResource> EditorIcons::GetBrandTexture(bool beam)
{
	std::string const name  = beam ? "@brand-beam" : "@brand-glyph";
	auto              found = m_icons.find(name);
	if (found != m_icons.end())
	{
		return found->second;
	}

	auto           image = CreateRef<Image>();
	uint8_t const* bytes = beam ? BuiltinBrand::Beam : BuiltinBrand::Glyph;
	size_t const   size  = beam ? sizeof(BuiltinBrand::Beam) : sizeof(BuiltinBrand::Glyph);
	if (!image->LoadFromMemory(std::vector<uint8_t>(bytes, bytes + size)))
	{
		return {};
	}

	auto texture     = CreateRef<TextureResource>();
	texture->m_image = image;
	if (!texture->InitGPUResources())
	{
		return {};
	}

	m_icons[name] = texture;
	return texture;
}

void EditorIcons::Shutdown()
{
	m_icons.clear();
}
