#include "MingEngine/Editor/UI/EditorIcons.hpp"

#include "MingEngine/Editor/Icons/BuiltinIcons.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

#include "ThirdParty/stb/stb_image.h"

namespace
{
ImTextureID ToImTextureId(GPUTexture* texture)
{
	if (texture == nullptr)
	{
		return {};
	}

	return (ImTextureID)(intptr_t)texture->GetShaderResourceView();
}

ImVec2 GetEditorIconUv0() { return ImVec2(0.f, 1.f); }

ImVec2 GetEditorIconUv1() { return ImVec2(1.f, 0.f); }
} // namespace

// TODO: Replace with ref-counted storage when available.
std::unordered_map<std::string, GPUTexture*> EditorIcons::s_iconTextures;

void EditorIcons::Startup()
{
	// Reserved for future initialization.
}

void EditorIcons::Shutdown()
{
	for (auto& pair : s_iconTextures)
	{
		if (g_engine != nullptr && g_engine->m_renderer != nullptr)
		{
			g_engine->m_renderer->DestroyTexture(pair.second);
		}
	}
	s_iconTextures.clear();
}

GPUTexture* EditorIcons::GetOrCreateIconTexture(std::string const& iconName)
{
	if (iconName.empty())
	{
		return nullptr;
	}

	auto const found = s_iconTextures.find(iconName);
	if (found != s_iconTextures.end())
	{
		return found->second;
	}

	BuiltinIcons::IconData const iconData = BuiltinIcons::Find(iconName);
	if (!iconData)
	{
		s_iconTextures[iconName] = nullptr;
		return nullptr;
	}

	int width    = 0;
	int height   = 0;
	int channels = 0;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* pixels = stbi_load_from_memory(
		iconData.m_data, static_cast<int>(iconData.m_size), &width, &height, &channels, STBI_rgb_alpha);
	stbi_set_flip_vertically_on_load(false);
	if (pixels == nullptr)
	{
		s_iconTextures[iconName] = nullptr;
		return nullptr;
	}

	GPUTexture* texture =
		g_engine->m_renderer->CreateGPUTexture(iconName.c_str(), IntVec2(width, height), STBI_rgb_alpha, pixels);

	stbi_image_free(pixels);

	s_iconTextures[iconName] = texture;
	return texture;
}

GPUTexture* EditorIcons::GetIconTexture(std::string const& iconName, std::string const& fallbackIconName)
{
	GPUTexture* texture = GetOrCreateIconTexture(iconName);
	if (texture != nullptr)
	{
		return texture;
	}

	if (!fallbackIconName.empty())
	{
		return GetOrCreateIconTexture(fallbackIconName);
	}

	return nullptr;
}

ImTextureID EditorIcons::GetIconId(std::string const& iconName, std::string const& fallbackIconName)
{
	return ToImTextureId(GetIconTexture(iconName, fallbackIconName));
}

bool EditorIcons::RenderIcon(std::string const& iconName, ImVec2 size, std::string const& fallbackIconName)
{
	ImTextureID textureId = GetIconId(iconName, fallbackIconName);
	if (textureId == ImTextureID{})
	{
		return false;
	}

	ImGui::Image(textureId, size, GetEditorIconUv0(), GetEditorIconUv1());
	return true;
}

void EditorIcons::AddImage(ImDrawList* drawList, ImTextureID textureId, ImVec2 min, ImVec2 max)
{
	if (drawList == nullptr || textureId == ImTextureID{})
	{
		return;
	}

	drawList->AddImage(textureId, min, max, GetEditorIconUv0(), GetEditorIconUv1());
}
