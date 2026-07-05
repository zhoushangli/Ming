#pragma once

#include "ThirdParty/imgui/imgui.h"

#include <string>

class GPUTexture;

class EditorIcons
{
public:
	static GPUTexture* GetIconTexture(std::string const& iconName);
	static GPUTexture* GetClassIconTexture(std::string const& className);

	static ImTextureID GetIconId(std::string const& iconName);
	static ImTextureID GetClassIconId(std::string const& className);

	static bool RenderIcon(std::string const& iconName, ImVec2 size);
	static bool RenderClassIcon(std::string const& className, ImVec2 size);

	static void AddImage(ImDrawList* drawList, ImTextureID textureId, ImVec2 min, ImVec2 max);
};
