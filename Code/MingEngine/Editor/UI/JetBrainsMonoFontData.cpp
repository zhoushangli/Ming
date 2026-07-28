#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"

#include "MingEngine/Editor/UI/JetBrainsMonoFont.cpp"

namespace JetBrainsMonoFont
{
unsigned char const* GetData() { return JetBrainsMono_compressed_data; }

unsigned int GetSize() { return JetBrainsMono_compressed_size; }
} // namespace JetBrainsMonoFont
