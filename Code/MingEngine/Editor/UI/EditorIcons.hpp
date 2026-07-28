#pragma once

#include "ThirdParty/imgui/imgui.h"

#include <string>
#include <unordered_map>

class GPUTexture;

class EditorIcons
{
public:
	// TODO: EditorIcons manages raw GPUTexture* in a static map loaded via stb_image.
	// Startup/Shutdown is a temporary measure; when the engine supports Ref<GPUTexture> or
	// a resource handle, remove explicit lifecycle and rely on ref-counted lifetime.
	static void Startup();
	static void Shutdown();

	static GPUTexture* GetIconTexture(std::string const& iconName, std::string const& fallbackIconName = "");
	static ImTextureID GetIconId(std::string const& iconName, std::string const& fallbackIconName = "");
	static bool        RenderIcon(std::string const& iconName, ImVec2 size, std::string const& fallbackIconName = "");
	static void        AddImage(ImDrawList* drawList, ImTextureID textureId, ImVec2 min, ImVec2 max);

private:
	static GPUTexture* GetOrCreateIconTexture(std::string const& iconName);

	// TODO: Replace with ref-counted storage when available.
	static std::unordered_map<std::string, GPUTexture*> s_iconTextures;
};
