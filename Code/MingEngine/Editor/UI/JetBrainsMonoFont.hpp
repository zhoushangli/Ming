#pragma once

#include "MingEngine/Scene/Resource/FontResource.hpp"

class Node;

namespace JetBrainsMonoFont
{
// Return the embedded compressed JetBrains Mono font bytes.
// e.g. GetData() and GetSize() expose the embedded byte buffer.
unsigned char const* GetData();
unsigned int         GetSize();
} // namespace JetBrainsMonoFont
