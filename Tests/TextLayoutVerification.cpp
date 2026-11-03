#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Window.hpp"
#include "MingEngine/Scene/GUI/Label.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/HBoxContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Scene/GUI/MarginContainer.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/ColorRect.hpp"
#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "RenderReadback.hpp"
#include "TextLayoutTestFont.inc"
#include "ThirdParty/GLFW/glfw3.h"

#undef CreateFont

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <functional>
#include <thread>

namespace
{
using Check = std::function<void(bool, char const*)>;

bool Near(float a, float b)
{
	return std::abs(a - b) < 0.01f;
}

String LineText(TextLine const& line)
{
	String text;
	for (auto const& glyph : line.m_glyphs)
	{
		char32_t const cp[] = { glyph.m_codePoint, 0 };
		text += cp;
	}

	return text;
}

void VerifyText(TextServer& server, RID font, RID fallback, Check const& check)
{
	auto shape = [&](String const& text)
	{
		return server.GetTextLine(font, text, 20.f);
	};
	auto lines = [&](String const& text, float width, AutowrapMode wrap = AutowrapMode::WordSmart)
	{
		return server.GetTextLines(font, text, 20.f, width, wrap);
	};
	auto original = shape("AVeryLongFileName.texture");
	check(LineText(shape("first\nsecond")) == "first", "single-line API retains its contract");
	check(
		server.GetTextLines(RID::Invalid, "text", 20.f, 100.f, AutowrapMode::Off).empty(),
		"invalid font has no layout");
	check(
		LineText(server.TrimTextLine(font, original, 20.f, 1.f, TextOverrunBehavior::TrimNothing))
			== LineText(original),
		"Trim Nothing retains all characters");
	check(
		LineText(server.TrimTextLine(font, original, 20.f, original.m_width, TextOverrunBehavior::EllipsisAlways))
			== LineText(original),
		"exact fit does not add ellipsis");
	auto trimmed =
		server.TrimTextLine(font, original, 20.f, shape("AVery").m_width, TextOverrunBehavior::TrimCharacters);
	check(LineText(trimmed) == "AVery", "character trimming keeps the longest complete prefix");
	float const ellipsisWidth = shape(String(U"\u2026")).m_width;
	auto shortTrim = server.TrimTextLine(font, original, 20.f, ellipsisWidth, TextOverrunBehavior::EllipsisAlways);
	check(LineText(shortTrim) == String(U"\u2026"), "short result still displays ellipsis");
	check(
		server.TrimTextLine(font, original, 20.f, ellipsisWidth * 0.5f, TextOverrunBehavior::EllipsisAlways)
			.m_glyphs.empty(),
		"insufficient ellipsis width displays no text");
	for (float width : { 0.f, 1.f, 18.f, 57.f, 100.f })
	{
		for (auto mode : { TextOverrunBehavior::TrimCharacters, TextOverrunBehavior::EllipsisAlways })
		{
			auto const result = server.TrimTextLine(font, original, 20.f, width, mode);
			check(
				result.m_width <= width + 0.001f && Near(result.m_width, shape(LineText(result)).m_width),
				"trimmed glyph positions and measured width agree");
		}
	}

	auto        fallbackLine  = server.GetTextLine(fallback, "AAAAAA", 20.f);
	float const fallbackWidth = server.GetTextLine(fallback, "A...", 20.f).m_width;
	auto        fallbackTrim =
		server.TrimTextLine(fallback, fallbackLine, 20.f, fallbackWidth, TextOverrunBehavior::EllipsisAlways);
	check(
		server.GetGlyphIndex(fallback, U'\u2026') == 0 && LineText(fallbackTrim) == "A..."
			&& Near(fallbackTrim.m_width, fallbackWidth),
		"missing ellipsis uses three dots including prefix kerning");

	auto explicitLines = lines("a\r\n\rb\n", 1.f, AutowrapMode::Off);
	check(
		explicitLines.size() == 4 && LineText(explicitLines[0]) == "a" && explicitLines[1].m_glyphs.empty()
			&& LineText(explicitLines[2]) == "b" && explicitLines[3].m_glyphs.empty(),
		"CRLF, CR, empty and trailing lines survive");
	check(lines("", 100.f).size() == 1, "empty text retains one font-height line");
	auto words = lines("one   two", shape("one ").m_width);
	check(
		words.size() == 2 && LineText(words[0]) == "one" && LineText(words[1]) == "two",
		"word wrapping skips separator spaces");
	auto indent = lines("a\n  b", 1000.f);
	check(indent.size() == 2 && LineText(indent[1]) == "  b", "explicit newline preserves indentation");
	auto longWord = lines("abcdefgh", shape("abc").m_width);
	check(longWord.size() == 3 && LineText(longWord[0]) == "abc", "long words split by character");
	auto chinese = lines(String(U"\u4e00\u4e8c\u4e09"), shape(String(U"\u4e00")).m_width);
	check(chinese.size() == 3, "Chinese wraps between UTF-32 characters");
	check(lines("abc", 0.01f).size() == 3, "extremely narrow wrapping always consumes a character");
	check(lines("abc", 0.f).size() == 1, "unassigned width uses explicit lines only");
}

void VerifyLayout(Ref<FontResource> const& font, Check const& check)
{
	SceneTree tree;
	auto      theme = CreateRef<Theme>();
	theme->SetDefaultFont(font);
	theme->SetDefaultFontSize(20);
	theme->SetConstant("separation", "BoxContainer", 7);
	theme->SetConstant("margin_left", "MarginContainer", 5);
	theme->SetConstant("margin_right", "MarginContainer", 9);
	auto* root = MemNew<MarginContainer>();
	root->SetTheme(theme);
	tree.GetRoot()->AddNode(root);
	auto* rows = MemNew<VBoxContainer>();
	root->AddNode(rows);
	auto* label = MemNew<Label>();
	label->SetAutowrapMode(AutowrapMode::WordSmart);
	label->SetText("one two three four five six seven eight nine ten");
	rows->AddNode(label);
	auto* after = MemNew<Control>();
	after->SetCustomMinimumSize(Vector2(1.f, 12.f));
	rows->AddNode(after);
	auto layout = [&](float width)
	{
		root->UpdateSize(Vector2(width, 1.f));
		tree.FlushContainerUpdate();
	};
	layout(320.f);
	float const wide = label->GetInternalMinimumSize().y;
	layout(110.f);
	float const narrow = label->GetInternalMinimumSize().y;
	check(
		narrow > wide && Near(after->GetLocalPosition().y, narrow + 7.f),
		"nested resize increases height and moves following row");
	for (float width : { 320.f, 110.f, 320.f, 110.f })
	{
		layout(width);
		float const height = label->GetInternalMinimumSize().y;
		tree.FlushContainerUpdate();
		check(
			Near(height, width == 320.f ? wide : narrow) && Near(after->GetLocalPosition().y, height + 7.f),
			"alternating width converges without stale row heights");
	}

	label->SetClipText(true);
	check(Near(label->GetInternalMinimumSize().y, narrow), "Clip Text preserves full wrapped height");
	label->SetCustomMinimumSize(Vector2(33.f, 0.f));
	check(label->GetCombinedMinimumSize().x == 33.f, "explicit minimum width remains effective");
	label->SetCustomMinimumSize(Vector2::Zero);
	theme->SetConstant("line_spacing", "Label", 6);
	tree.FlushContainerUpdate();
	check(label->GetInternalMinimumSize().y > narrow, "live line spacing updates wrapped height");
	theme->SetDefaultFontSize(26);
	tree.FlushContainerUpdate();
	check(label->GetInternalMinimumSize().y > narrow, "live font size propagates through nested containers");
	auto replacement = FontResource::CreateDefault();
	theme->SetDefaultFont(replacement);
	tree.FlushContainerUpdate();
	check(label->GetFont() == replacement, "live font replacement reaches text cache");

	rows->SetSeparation(2.5f);
	theme->SetConstant("separation", "BoxContainer", 19);
	check(rows->GetSeparation() == 2.5f, "float separation override survives Theme updates");
	rows->RemoveSeparationOverride();
	tree.FlushContainerUpdate();
	check(
		rows->GetSeparation() == 19.f && Near(after->GetLocalPosition().y, label->GetSize().y + 19.f),
		"removing separation override restores themed layout");
	root->SetMarginLeft(1.25f);
	theme->SetConstant("margin_right", "MarginContainer", 15);
	check(
		root->GetMarginLeft() == 1.25f && root->GetMarginRight() == 15.f,
		"single-edge float override leaves other edges themed");
	root->SetMargins(1.5f, 2.5f, 3.5f, 4.5f);
	check(
		root->GetMarginTop() == 2.5f && root->GetMarginBottom() == 4.5f,
		"four-edge setter preserves float precision");
	root->RemoveMarginsOverride();
	check(root->GetMarginLeft() == 5.f && root->GetMarginRight() == 15.f, "removing margin overrides restores Theme");
	theme->SetConstant("separation", "BoxContainer", -3);
	theme->SetConstant("margin_left", "MarginContainer", -5);
	check(rows->GetSeparation() == 0.f && root->GetMarginLeft() == 0.f, "negative Theme dimensions clamp to zero");
	after->SetVisible(false);
	tree.FlushContainerUpdate();
	check(
		Near(rows->GetInternalMinimumSize().y, label->GetCombinedMinimumSize().y),
		"hidden children reserve no height or gap");

	auto* plain = MemNew<Label>();
	rows->AddNode(plain);
	plain->SetText("long filename");
	float const natural = plain->GetInternalMinimumSize().x;
	plain->SetText("a\nb\n");
	float const fontHeight = plain->GetFontSize();
	check(plain->GetInternalMinimumSize().y >= fontHeight * 3.f, "explicit empty lines contribute full label height");
	plain->SetText("long filename");
	plain->SetClipText(true);
	check(natural > 1.f && plain->GetInternalMinimumSize().x == 1.f, "Clip Text removes the natural width constraint");
	plain->SetClipContents(true);
	plain->SetClipText(false);
	check(plain->IsClippingContents(), "text clipping does not overwrite Control child clipping");
	plain->SetTextOverrunBehavior(TextOverrunBehavior::TrimCharacters);
	check(plain->GetInternalMinimumSize().x == 1.f, "trimming independently relaxes minimum width");
	plain->SetTextOverrunBehavior(TextOverrunBehavior::TrimNothing);
	check(Near(plain->GetInternalMinimumSize().x, natural), "default configuration restores natural width");
}

void VerifyRendering(
	Engine& engine, Ref<FontResource> const& fixture, std::filesystem::path const& project, Check const& check)
{
	SceneTree tree;
	auto*     viewport = static_cast<Viewport*>(tree.GetRoot());
	viewport->UpdateResolution(IntVec2(240, 180));
	auto* canvas = MemNew<Control>();
	viewport->AddNode(canvas);
	auto* background = MemNew<ColorRect>();
	background->SetColor(Color(0, 0, 0));
	background->UpdateSize(Vector2(240.f, 180.f));
	canvas->AddNode(background);
	for (int row = 0; row < 2; ++row)
	{
		auto* label = MemNew<Label>();
		label->SetFont(fixture);
		label->SetFontSize(20);
		label->SetFontColor(Color(255, 0, 0));
		label->SetText("AAAA");
		label->SetTextOverrunBehavior(TextOverrunBehavior::TrimCharacters);
		label->SetClipText(row == 0);
		label->SetPosition(Vector2(40.f, 40.f + row * 40.f));
		label->UpdateSize(Vector2(12.f, 20.f));
		canvas->AddNode(label);
	}

	auto icon     = CreateRef<TextureResource>();
	icon->m_image = CreateRef<Image>();
	std::vector<uint8_t> pixels(16 * 16 * 4, 0);
	for (size_t i = 0; i < pixels.size(); i += 4)
	{
		pixels[i + 1] = 255;
		pixels[i + 3] = 255;
	}

	icon->m_image->LoadFromRGBA8(IntVec2(16, 16), std::move(pixels));
	icon->InitGPUResources();
	auto* button = MemNew<Button>();
	button->SetFont(fixture);
	button->SetFontSize(20);
	button->SetIcon(icon);
	button->SetText("AAAAAAAAAA");
	float const buttonNatural = button->GetInternalMinimumSize().x;
	button->SetTextOverrunBehavior(TextOverrunBehavior::TrimCharacters);
	float const buttonMinimum = button->GetInternalMinimumSize().x;
	check(
		buttonMinimum < buttonNatural && buttonMinimum >= 16.f + 6.f + 1.f,
		"button shrink retains icon, spacing and margins");
	button->SetCustomMinimumSize(Vector2(60.f, 0.f));
	check(button->GetCombinedMinimumSize().x >= 60.f, "button keeps its explicit minimum width");
	button->SetCustomMinimumSize(Vector2::Zero);
	button->SetClipText(true);
	button->SetPosition(Vector2(40.f, 120.f));
	button->UpdateSize(Vector2(64.f, 30.f));
	canvas->AddNode(button);
	engine.BeginFrame();
	tree.UpdateScene(0.016f);
	engine.m_renderServer->Render();
	engine.EndFrame();
	auto* texture = engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	check(
		ReadPixel(texture, 39, 48).r == 0 && ReadPixel(texture, 54, 48).r == 0 && ReadPixel(texture, 42, 48).r > 200,
		"glyph clipping updates target and source rectangles");
	check(ReadPixel(texture, 39, 88).r > 200, "trimming without clipping preserves negative glyph bearing");
	check(ReadPixel(texture, 52, 135).g > 200, "narrow button keeps its icon visible");
	std::filesystem::create_directories(project.parent_path() / "Captures");
	SaveRenderedFrame(texture, project.parent_path() / "Captures" / "text-layout-clipping.ppm");
}

void Preview(Engine& engine)
{
	SceneTree tree;
	auto*     viewport = static_cast<Viewport*>(tree.GetRoot());
	auto      theme    = CreateRef<Theme>();
	theme->SetDefaultFont(FontResource::CreateDefault());
	theme->SetDefaultFontSize(18);
	theme->SetConstant("separation", "BoxContainer", 8);
	theme->SetConstant("margin_left", "MarginContainer", 20);
	theme->SetConstant("margin_right", "MarginContainer", 20);
	theme->SetConstant("margin_top", "MarginContainer", 20);
	auto* background = MemNew<ColorRect>();
	background->SetColor(Color(24, 24, 24));
	viewport->AddNode(background);
	auto* margin = MemNew<MarginContainer>();
	margin->SetTheme(theme);
	viewport->AddNode(margin);
	auto* rows = MemNew<VBoxContainer>();
	margin->AddNode(rows);
	auto addLabel = [&](char const* text)
	{
		auto* label = MemNew<Label>();
		label->SetText(text);
		label->SetClipText(true);
		rows->AddNode(label);
		return label;
	};
	addLabel("Text layout | resize window and update Theme");
	auto* actions = MemNew<HBoxContainer>();
	rows->AddNode(actions);
	auto* update = MemNew<Button>();
	update->SetText("Update Theme");
	actions->AddNode(update);
	auto* close = MemNew<Button>();
	close->SetText("Close preview");
	actions->AddNode(close);
	char const* names[] = { "Clip / Trim Nothing", "Trim Characters", "Ellipsis Always" };
	for (int i = 0; i < 3; ++i)
	{
		addLabel(names[i]);
		auto* panel = MemNew<PanelContainer>();
		rows->AddNode(panel);
		auto* text = MemNew<Label>();
		text->SetText("A very long filename - assets/textures/characters/player_animation_diffuse.texture - end");
		text->SetClipText(true);
		text->SetTextOverrunBehavior(static_cast<TextOverrunBehavior>(i));
		panel->AddNode(text);
	}

	addLabel("Wrapped description (following controls track its height)");
	auto* wrap = addLabel(
		"Ordinary text wraps at word boundaries. ALongUnbrokenFilenameStillWrapsCharacterByCharacter.\n\nExplicit blank lines and indentation remain:\n  Indented text after an explicit newline.");
	wrap->SetAutowrapMode(AutowrapMode::WordSmart);
	auto* button = MemNew<Button>();
	button->SetText("A long button title also reserves its own content margins before adding an ellipsis - end");
	button->SetClipText(true);
	button->SetTextOverrunBehavior(TextOverrunBehavior::EllipsisAlways);
	rows->AddNode(button);
	addLabel("End of content");
	bool large = false;
	bool done  = false;
	update->SetPressedCallback(
		[&]()
		{
			large = !large;
			theme->SetDefaultFontSize(large ? 24 : 18);
			theme->SetConstant("line_spacing", "Label", large ? 5 : 0);
			theme->SetConstant("separation", "BoxContainer", large ? 12 : 8);
			theme->SetConstant("margin_left", "MarginContainer", large ? 32 : 20);
		});
	close->SetPressedCallback(
		[&]()
		{
			done = true;
		});
	auto* window = engine.m_windowSystem->GetGLFWWindow();
	glfwSetWindowTitle(window, "Ming Text Layout Preview");
	glfwSetWindowSize(window, 760, 800);
	glfwRestoreWindow(window);
	while (!done && !glfwWindowShouldClose(window))
	{
		engine.BeginFrame();
		Vector2 const size(viewport->GetOutputResolution());
		background->UpdateSize(size);
		margin->UpdateSize(size);
		tree.UpdateScene(0.016f);
		engine.m_renderServer->Render();
		engine.EndFrame();
		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}
}
}

int main(int argc, char** argv)
{
	if (argc < 2 || argc > 3)
	{
		return 2;
	}

	bool const                  preview = argc == 3 && std::string(argv[2]) == "--preview";
	std::filesystem::path const project = std::filesystem::absolute(argv[1]);
	std::filesystem::create_directories(project);
	EngineConfig config;
	config.m_scriptSystemConfig.m_isEnabled        = false;
	config.m_audioConfig.m_isEnable                = false;
	config.m_networkConfig.m_isEnable              = false;
	config.m_fileSystemConfig.m_resourceRoot       = project;
	config.m_windowConfig.m_appName                = "Ming Text Layout Verification";
	config.m_windowConfig.m_isResizable            = true;
	config.m_windowConfig.m_isBorderlessFullscreen = false;
	Engine engine(config);
	ClassDatabase::Startup();
	RegisterAllTypes();
	engine.Startup();
	int  checks   = 0;
	int  failures = 0;
	auto check    = [&](bool result, char const* name)
	{
		++checks;
		if (!result)
		{
			++failures;
			std::printf("FAIL: %s\n", name);
		}

		std::fflush(stdout);
	};
	{
		auto           font       = FontResource::CreateDefault();
		auto           editorFont = FontResource::CreateDefault(true);
		TextLine const editorLine = engine.m_textServer->GetTextLine(editorFont->GetFontRID(), "W", 12.f);
		check(Near(editorLine.m_width, 7.2f), "12 pixel editor em matches the CSS monospace advance");
		TextLineConfig const legacyMetrics = engine.m_textServer->GetFontMetrics(font->GetFontRID(), 12.f);
		check(
			Near(legacyMetrics.m_ascent - legacyMetrics.m_descent, 12.f),
			"default font loading preserves the existing pixel height sizing contract");
		auto fixture = CreateRef<FontResource>();
		fixture->SetFontRID(engine.m_textServer->CreateFont(
			std::vector<uint8_t>(std::begin(kTextLayoutFont), std::end(kTextLayoutFont))));
		if (preview)
		{
			Preview(engine);
		}
		else
		{
			VerifyText(*engine.m_textServer, font->GetFontRID(), fixture->GetFontRID(), check);
			VerifyLayout(font, check);
			VerifyRendering(engine, fixture, project, check);
		}
	}

	ThemeDatabase::Shutdown();
	ResourceLoader::Shutdown();
	engine.Shutdown();
	ClassDatabase::Shutdown();
	std::printf("TEXT_LAYOUT checks=%d failures=%d\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
