#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

#include "MingEngine/Editor/UI/JetBrainsMonoFont.cpp"


namespace JetBrainsMonoFont
{
unsigned char const* GetData()
{
	return JetBrainsMono_data;
}

unsigned int GetSize()
{
	return JetBrainsMono_size;
}
Ref<FontResource> Create()
{
	auto font = CreateRef<FontResource>();
	if (g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		font->SetFontRID(g_engine->m_textServer->CreateFont(
			std::vector<uint8_t>(GetData(), GetData() + GetSize()), true));
	}

	return font;
}
} // namespace JetBrainsMonoFont
