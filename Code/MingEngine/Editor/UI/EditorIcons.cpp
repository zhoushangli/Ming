#include "MingEngine/Editor/UI/EditorIcons.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/Texture.hpp"

#include <filesystem>
#include <unordered_map>

namespace
{
constexpr char const* kIconDirectory     = "Data/Icon/";
constexpr char const* kIconExtension     = ".png";
constexpr char const* kFallbackClassIcon = "Node";

std::string BuildIconPath(std::string const& iconName)
{
	return std::string(kIconDirectory) + iconName + kIconExtension;
}

bool DoesIconFileExist(std::string const& iconName)
{
	std::error_code             errorCode;
	std::filesystem::path const path = BuildIconPath(iconName);
	return std::filesystem::exists(path, errorCode) && std::filesystem::is_regular_file(path, errorCode);
}

ImTextureID ToImTextureId(Texture* texture)
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

Texture* EditorIcons::GetIconTexture(std::string const& iconName)
{
	static std::unordered_map<std::string, Texture*> s_iconTextures;

	if (iconName.empty())
	{
		return nullptr;
	}

	auto const found = s_iconTextures.find(iconName);
	if (found != s_iconTextures.end())
	{
		return found->second;
	}

	Texture* texture = nullptr;
	if (DoesIconFileExist(iconName) && g_engine != nullptr && g_engine->m_renderer != nullptr)
	{
		std::string const iconPath = BuildIconPath(iconName);
		texture                    = g_engine->m_renderer->CreateOrGetTexture(iconPath.c_str());
	}

	s_iconTextures[iconName] = texture;
	return texture;
}

Texture* EditorIcons::GetClassIconTexture(std::string const& className)
{
	static std::unordered_map<std::string, Texture*> s_classIconTextures;

	if (className.empty())
	{
		return GetIconTexture(kFallbackClassIcon);
	}

	auto const found = s_classIconTextures.find(className);
	if (found != s_classIconTextures.end())
	{
		return found->second;
	}

	Texture* texture = GetIconTexture(className);
	if (texture != nullptr)
	{
		s_classIconTextures[className] = texture;
		return texture;
	}

	Texture* fallbackTexture       = GetIconTexture(kFallbackClassIcon);
	s_classIconTextures[className] = fallbackTexture;
	return fallbackTexture;
}

ImTextureID EditorIcons::GetIconId(std::string const& iconName) { return ToImTextureId(GetIconTexture(iconName)); }

ImTextureID EditorIcons::GetClassIconId(std::string const& className)
{
	return ToImTextureId(GetClassIconTexture(className));
}

bool EditorIcons::RenderIcon(std::string const& iconName, ImVec2 size)
{
	ImTextureID textureId = GetIconId(iconName);
	if (textureId == ImTextureID{})
	{
		return false;
	}

	ImGui::Image(textureId, size, GetEditorIconUv0(), GetEditorIconUv1());
	return true;
}

bool EditorIcons::RenderClassIcon(std::string const& className, ImVec2 size)
{
	ImTextureID textureId = GetClassIconId(className);
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
