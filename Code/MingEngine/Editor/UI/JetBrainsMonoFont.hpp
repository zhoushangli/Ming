#pragma once

#include "MingEngine/Scene/Resource/FontResource.hpp"

class Node;

namespace JetBrainsMonoFont
{
// Return the embedded compressed JetBrains Mono font bytes.
// e.g. ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(GetData(), GetSize(), 24.f)
unsigned char const* GetData();
unsigned int         GetSize();
Ref<FontResource>    CreateNativeFont();
void                 ApplyNativeFont(Node* root, Ref<FontResource> const& font);
} // namespace JetBrainsMonoFont
