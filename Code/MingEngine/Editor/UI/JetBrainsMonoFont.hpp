#pragma once

namespace JetBrainsMonoFont
{
// Return the embedded compressed JetBrains Mono font bytes.
// e.g. ImGui::GetIO().Fonts->AddFontFromMemoryCompressedTTF(GetData(), GetSize(), 24.f)
unsigned char const* GetData();
unsigned int         GetSize();
} // namespace JetBrainsMonoFont
