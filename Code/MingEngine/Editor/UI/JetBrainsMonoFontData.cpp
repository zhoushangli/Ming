#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"

#include "MingEngine/Editor/UI/JetBrainsMonoFont.cpp"
#include "MingEngine/Editor/UI/JetBrainsMonoNativeData.inc"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/Tree.hpp"

namespace JetBrainsMonoFont
{
Ref<FontResource> CreateNativeFont()
{
	auto font = CreateRef<FontResource>();
	font->SetFontRID(g_engine->m_textServer->CreateFont(
		std::vector<uint8_t>(std::begin(kNativeFontData), std::end(kNativeFontData))));
	return font;
}

void ApplyNativeFont(Node* root, Ref<FontResource> const& font)
{
	if (auto* label = dynamic_cast<Label*>(root))
	{
		label->SetFont(font);
	}
	else if (auto* edit = dynamic_cast<LineEdit*>(root))
	{
		edit->SetFont(font);
	}
	else if (auto* button = dynamic_cast<Button*>(root))
	{
		button->SetFont(font);
	}
	else if (auto* tree = dynamic_cast<Tree*>(root))
	{
		tree->SetFont(font);
	}

	for (Node* child : root->GetChildren())
	{
		ApplyNativeFont(child, font);
	}
}

unsigned char const* GetData()
{
	return JetBrainsMono_compressed_data;
}

unsigned int GetSize()
{
	return JetBrainsMono_compressed_size;
}
} // namespace JetBrainsMonoFont
