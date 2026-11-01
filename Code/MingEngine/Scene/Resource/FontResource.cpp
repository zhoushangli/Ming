#include "MingEngine/Scene/Resource/FontResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/TextServer.hpp"

#include <utility>
#include "MingEngine/Scene/Resource/DefaultFontData.inc"

FontResource::~FontResource()
{
	SetFontRID(RID::Invalid);
}

void FontResource::SetFontRID(RID rid)
{
	if (m_fontRID == rid)
	{
		return;
	}

	if (g_engine != nullptr && g_engine->m_textServer != nullptr && m_fontRID.IsValid())
	{
		g_engine->m_textServer->FreeFont(m_fontRID);
	}

	m_fontRID = rid;
}

bool FontResource::CopyFrom(Resource&& other)
{
	FontResource* source = dynamic_cast<FontResource*>(&other);
	if (source == nullptr)
	{
		return false;
	}

	MoveBaseFrom(std::move(other));
	SetFontRID(source->m_fontRID);
	source->m_fontRID = RID::Invalid;
	return true;
}
Ref<FontResource> FontResource::CreateDefault(bool sizeInEm)
{
	auto font = CreateRef<FontResource>();
	if (g_engine != nullptr && g_engine->m_textServer != nullptr)
	{
		font->SetFontRID(g_engine->m_textServer->CreateFont(
			std::vector<uint8_t>(std::begin(kNativeFontData), std::end(kNativeFontData)), sizeInEm));
	}

	return font;
}
