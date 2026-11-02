#pragma once

#include "MingEngine/Scene/Resource/FontResource.hpp"

class Node;

namespace JetBrainsMonoFont
{
// Return the embedded JetBrains Mono font bytes.
// e.g. GetData() and GetSize() expose the embedded byte buffer.
unsigned char const* GetData();
unsigned int         GetSize();
Ref<FontResource>    Create();
} // namespace JetBrainsMonoFont
