#pragma once

#include "MingEngine/Scene/Resource/TextureResource.hpp"
#include "MingEngine/Core/Render/Color.hpp"
#include <string>
#include <map>

class Control;

class EditorIcons
{
public:
	static Ref<TextureResource> GetIconTexture(std::string const& name, std::string const& fallback = {});
	static Color                GetIconColor(std::string const& name, Control const* source);
	static Ref<TextureResource> GetBrandTexture(bool beam);
	static void                 Shutdown();

private:
	static std::map<std::string, Ref<TextureResource>> m_icons;
};
