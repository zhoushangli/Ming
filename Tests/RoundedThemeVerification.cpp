#define NOMINMAX

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Application/App.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/UI/EditorUI.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/StringUtils.hpp"
#include "MingEngine/Scene/RegisterAllTypes.hpp"
#include "MingEngine/Scene/GUI/DefaultTheme.hpp"
#include "MingEngine/Scene/GUI/Button.hpp"
#include "MingEngine/Scene/GUI/LineEdit.hpp"
#include "MingEngine/Scene/GUI/PanelContainer.hpp"
#include "MingEngine/Scene/GUI/ProgressBar.hpp"
#include "MingEngine/Scene/GUI/TabContainer.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/VBoxContainer.hpp"
#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/Popup/EditorPopupUtils.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "ThirdParty/GLFW/glfw3.h"

#include "MingEngine/Engine/Render/GPUTexture.hpp"
#include <fstream>
#include <cstdio>
#include <limits>
#include <stdexcept>

// Inspect CPU geometry without exposing test-only engine APIs.
// e.g. The runner excludes Control.obj and RenderServer.obj when linking this translation unit.
#include "MingEngine/Scene/GUI/Control.cpp"
#include "MingEngine/Engine/Render/RenderServer.cpp"

namespace
{
int  checks = 0;
void Check(bool passed, char const* message)
{
	++checks;
	if (!passed)
	{
		throw std::runtime_error(message);
	}
}

bool Near(float a, float b)
{
	return std::abs(a - b) < 0.001f;
}

void VerifyResources()
{
	auto box = CreateRef<ThemeStyleBox>();
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 0.f, "custom styles default to square");
	box->SetCornerRadiusAll(8.f);
	box->SetCornerRadius(ThemeStyleBox::Corner::TopLeft, 2.f);
	box->SetCornerRadius(ThemeStyleBox::Corner::TopRight, -1.f);
	box->SetCornerRadius(ThemeStyleBox::Corner::BottomRight, std::numeric_limits<float>::infinity());
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 2.f, "independent corners");
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::TopRight) == 0.f, "negative radius clamps");
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::BottomRight) == 8.f, "invalid radius ignored");
	box->SetCornerRadiusAll(std::numeric_limits<float>::quiet_NaN());
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::BottomLeft) == 8.f, "invalid uniform radius ignored");

	auto theme = CreateRef<Theme>();
	theme->SetStyleBox("normal", "Button", box);
	theme->SetStyleBox("hover", "Button", box);
	auto copy = theme->Duplicate();
	Check(copy->GetStyleBox("normal", "Button") == copy->GetStyleBox("hover", "Button"), "duplicate preserves aliases");
	copy->GetStyleBox("normal", "Button")->SetCornerRadiusAll(4.f);
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 2.f, "duplicate isolates base");
	auto moved = CreateRef<ThemeStyleBox>();
	Check(moved->MoveFrom(std::move(*box.Get())), "move style data");
	Check(moved->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 2.f, "move preserves radius");
	Check(box->GetCornerRadius(ThemeStyleBox::Corner::BottomLeft) == 0.f, "move resets source radii");

	auto* control = MemNew<Control>();
	control->SetTheme(theme);
	int refreshes = 0;
	control->AddThemeChangedCallback(
		[&refreshes]()
		{
			++refreshes;
		});
	refreshes = 0;
	box->SetCornerRadiusAll(6.f);
	Check(refreshes == 1, "shared style mutation refreshes user once");
	box->SetCornerRadiusAll(6.f);
	Check(refreshes == 1, "unchanged radius is silent");
	Check(
		control->GetThemeStyleBox("normal", "Button")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 6.f,
		"theme lookup sees new radius");
	MemDelete(control);
}

float SampleStyleAlpha(std::vector<Vertex> const& vertices, float x, float y, int& layers)
{
	float alpha = 0.f;
	layers      = 0;
	for (size_t i = 0; i < vertices.size(); i += 3)
	{
		auto const& a    = vertices[i];
		auto const& b    = vertices[i + 1];
		auto const& c    = vertices[i + 2];
		float const area = (b.m_position.y - c.m_position.y) * (a.m_position.x - c.m_position.x)
						   + (c.m_position.x - b.m_position.x) * (a.m_position.y - c.m_position.y);
		float const wa   = ((b.m_position.y - c.m_position.y) * (x - c.m_position.x)
							+ (c.m_position.x - b.m_position.x) * (y - c.m_position.y))
						   / area;
		float const wb   = ((c.m_position.y - a.m_position.y) * (x - c.m_position.x)
							+ (a.m_position.x - c.m_position.x) * (y - c.m_position.y))
						   / area;
		float const wc   = 1.f - wa - wb;
		if (wa > 0.00001f && wb > 0.00001f && wc > 0.00001f)
		{
			float const value = wa * a.m_color.a + wb * b.m_color.a + wc * c.m_color.a;
			if (value > 0.001f)
			{
				++layers;
				alpha += value;
			}
		}
	}
	return alpha;
}

void VerifyGeometry()
{
	auto box = CreateRef<ThemeStyleBox>();
	box->SetBackgroundColor(Color(80.f / 255.f, 100.f / 255.f, 120.f / 255.f, 128.f / 255.f));
	box->SetBorderColor(Color(210.f / 255.f, 170.f / 255.f, 110.f / 255.f, 160.f / 255.f));
	AABB2 const rect(0.f, 0.f, 80.f, 30.f);
	for (float radius : { 0.f, 2.f, 4.f, 8.f, 10000.f })
	{
		box->SetCornerRadiusAll(radius);
		for (float border : { 0.f, 1.f, 6.f, 100.f })
		{
			box->SetBorderWidth(ThemeStyleBox::Side::Top, border);
			box->SetBorderWidth(ThemeStyleBox::Side::Left, border * 0.5f);
			auto vertices = BuildRoundedStyleVertices(*box.Get(), rect);
			bool valid    = !vertices.empty() && vertices.size() % 3 == 0;
			for (auto const& v : vertices)
			{
				valid = valid && std::isfinite(v.m_position.x) && std::isfinite(v.m_position.y)
						&& v.m_position.x >= -0.501f && v.m_position.x <= 80.501f && v.m_position.y >= -0.501f
						&& v.m_position.y <= 30.501f;
			}
			Check(valid, "finite fitted geometry for radius/border limits");
			bool noOverlap = true;
			for (float y = 0.173f; y < 30.f; y += 2.37f)
			{
				for (float x = 0.237f; x < 80.f; x += 2.19f)
				{
					int layers = 0;
					SampleStyleAlpha(vertices, x, y, layers);
					if (noOverlap && layers > 1)
					{
						std::printf("overlap radius=%g border=%g x=%g y=%g layers=%d\n", radius, border, x, y, layers);
					}
					noOverlap = noOverlap && layers <= 1;
				}
			}
			Check(noOverlap, "translucent border and fill do not overlap");
		}
	}

	box->SetCornerRadiusAll(0.f);
	box->SetCornerRadius(ThemeStyleBox::Corner::TopLeft, 8.f);
	box->SetCornerRadius(ThemeStyleBox::Corner::TopRight, 4.f);
	box->SetBorderWidth(ThemeStyleBox::Side::Left, 0.f);
	box->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	auto topCorners   = BuildRoundedStyleVertices(*box.Get(), rect);
	int  cornerLayers = 0;
	Check(
		SampleStyleAlpha(topCorners, 0.137f, 0.173f, cornerLayers) == 0.f,
		"top rounded corner excludes rectangle corner");
	Check(SampleStyleAlpha(topCorners, 0.637f, 29.173f, cornerLayers) > 0.f, "square bottom corner remains filled");
	box->SetCornerRadiusAll(8.f);
	Check(BuildRoundedStyleVertices(*box.Get(), AABB2(0.f, 0.f, 0.f, 1.f)).empty(), "empty rectangles skipped");
	auto tiny = BuildRoundedStyleVertices(*box.Get(), AABB2(0.f, 0.f, 0.2f, 0.1f));
	Check(!tiny.empty(), "subpixel rectangle remains valid");
	box->SetBackgroundColor(Color::Transparent);
	box->SetBorderColor(Color::Transparent);
	Check(BuildRoundedStyleVertices(*box.Get(), rect).empty(), "invisible styles emit no geometry");

	std::vector<Vertex> triangle = { Vertex(Vector3(0.f, 0.f, 0.f), Color(0.f, 0.f, 0.f, 0.f), Vector2(0.f, 0.f)),
									 Vertex(Vector3(10.f, 0.f, 0.f), Color(200.f / 255.f, 100.f / 255.f, 50.f / 255.f, 200.f / 255.f), Vector2(1.f, 0.f)),
									 Vertex(Vector3(0.f, 10.f, 0.f), Color::White, Vector2(0.f, 1.f)) };
	AABB2 const         clip(7.f, 3.f, 11.f, 9.f);
	auto                clipped = PrepareCanvasTriangles(triangle, Vector2(2.f, 3.f), &clip);
	Check(!clipped.empty(), "translated triangle survives partial clip");
	bool inside       = true;
	bool interpolated = false;
	for (auto const& v : clipped)
	{
		inside = inside && v.m_position.x >= 6.999f && v.m_position.x <= 11.001f && v.m_position.y >= 2.999f
				 && v.m_position.y <= 9.001f;
		if (Near(v.m_position.x, 7.f) && Near(v.m_position.y, 3.f))
		{
			interpolated = Near(v.m_uv.x, 0.5f) && v.m_color.r == 100.f / 255.f && v.m_color.a == 100.f / 255.f;
		}
	}
	Check(inside && interpolated, "clip interpolates position UV color and alpha");
	AABB2 const outside(30.f, 30.f, 40.f, 40.f);
	Check(PrepareCanvasTriangles(triangle, Vector2::Zero, &outside).empty(), "fully clipped triangle skipped");
	auto translated = PrepareCanvasTriangles(triangle, Vector2(3.f, 4.f), nullptr);
	Check(
		Near(translated[1].m_position.x, 13.f) && translated[1].m_color == triangle[1].m_color,
		"unclipped translation preserves attributes");
}

void VerifyStandards()
{
	auto defaults = CreateDefaultTheme();
	auto editor   = EditorThemeManager::CreateTheme();
	Check(editor->GetStyleBox("normal", "MenuBarButton")->GetBackgroundColor().a == 0.f,
		"editor menu idle background stays transparent");
	for (auto const& theme : { defaults, editor })
	{
		Check(
			theme->GetStyleBox("normal", "Button")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 4.f,
			"standard control radius");
		Check(
			theme->GetStyleBox("focus", "MenuBarButton")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 2.f,
			"compact focus matches compact control");
		Check(
			theme->GetStyleBox("panel", "PanelContainer")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 6.f,
			"independent panel radius");
		Check(
			theme->GetStyleBox("tab_selected", "TabContainer")->GetCornerRadius(ThemeStyleBox::Corner::BottomLeft)
				== 0.f,
			"joined tab lower edge stays square");
	}

	Check(
		editor->GetStyleBox("panel", "EditorDialog")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 8.f,
		"floating dialog radius");
	Check(
		editor->GetStyleBox("panel", "EditorLogArea")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 0.f,
		"joined log area stays square");
	auto alias = editor->GetStyleBox("tab_selected", "TabContainer");
	Check(editor->GetStyleBox("pressed", "EditorOutputTab") == alias, "output tabs retain shared tab styles");
	for (float scale : { 1.f, 1.25f, 1.5f, 2.f })
	{
		auto scaled = EditorThemeManager::CreateScaledTheme(editor, scale);
		Check(
			Near(scaled->GetStyleBox("normal", "Button")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft), 4.f * scale),
			"control scales from base");
		Check(
			Near(
				scaled->GetStyleBox("pressed", "EditorOutputTab")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft),
				4.f * scale),
			"shared tab style scales once");
		Check(
			editor->GetStyleBox("panel", "EditorDialog")->GetCornerRadius(ThemeStyleBox::Corner::TopLeft) == 8.f,
			"scaling preserves base theme");
	}
}

Label* AddPreviewLabel(Control* root, char const* text, float x, float y)
{
	auto* label = MemNew<Label>();
	label->SetText(text);
	label->SetPosition(Vector2(x, y));
	label->UpdateSize(Vector2(900.f, 24.f));
	root->AddNode(label);
	return label;
}

void PopulatePreview(Engine& engine, float scale)
{
	auto* root = MemNew<Control>();
	root->SetName("RoundedThemePreview");
	root->UpdateSize(Vector2(1450.f, 1000.f));
	root->SetTheme(EditorThemeManager::CreateScaledTheme(EditorThemeManager::CreateTheme(), scale));
	engine.m_sceneSystem->GetSceneTree()->GetRoot()->AddNode(root);
	AddPreviewLabel(root, "Rounded Theme: 0 structure / 2 compact / 4 control / 6 panel / 8 popup", 30.f, 20.f);
	char const* types[] = { "EditorPanelAction", "Button",         "EditorPrimaryButton",
							"LineEdit",          "PanelContainer", "EditorMenuPopup" };
	char const* names[] = { "Compact 2", "Button 4", "Primary 4", "Field 4", "Panel 6", "Popup 8" };
	for (int i = 0; i < 6; ++i)
	{
		float const x = 30.f + i * 225.f;
		AddPreviewLabel(root, names[i], x, 65.f);
		int stateIndex = 0;
		for (char const* state : { "normal", "hover", "pressed", "disabled", "focus" })
		{
			std::string const displayState = i == 3 && std::string(state) == "pressed" ? "read_only" : state;
			auto              style        = root->GetThemeStyleBox(i >= 4 ? "panel" : displayState, types[i]);
			auto*             sample       = MemNew<PanelContainer>();
			sample->AddThemeStyleBoxOverride("panel", style);
			sample->SetPosition(Vector2(x, 100.f + stateIndex * 80.f));
			sample->UpdateSize(Vector2(180.f, 48.f));
			root->AddNode(sample);
			auto* label = MemNew<Label>();
			label->SetText(displayState);
			sample->AddNode(label);
			std::string const fontState =
				displayState == "normal" || displayState == "focus" ? "font_color" : "font_" + displayState + "_color";
			label->SetFontColor(root->GetThemeColor(i >= 4 ? "font_color" : fontState, types[i]));
			++stateIndex;
		}
	}

	auto* button = MemNew<Button>();
	button->SetName("OpenDialog");
	button->SetText("Open dialog");
	button->SetPosition(Vector2(30.f, 520.f));
	button->UpdateSize(Vector2(230.f, 45.f));
	root->AddNode(button);
	button->SetPressedCallback(
		[root]()
		{
			VBoxContainer* content = nullptr;
			auto*          popup   = EditorPopupUtils::Create(root, "Rounded dialog", content, true);
			EditorPopupUtils::AddLabel(content, "Header and body preserve the outer corners.");
			auto* input = MemNew<LineEdit>();
			input->SetPlaceholder("Type here to check focus and editing");
			content->AddNode(input);
			popup->Open(root);
			EditorPopupUtils::Layout(popup, Vector2(560.f, 300.f));
		});
	auto* input = MemNew<LineEdit>();
	input->SetName("InputField");
	input->SetPlaceholder("Interactive LineEdit");
	input->SetPosition(Vector2(300.f, 520.f));
	input->UpdateSize(Vector2(300.f, 45.f));
	root->AddNode(input);
	auto* tabs = MemNew<TabContainer>();
	tabs->SetName("PreviewTabs");
	tabs->SetPosition(Vector2(30.f, 610.f));
	tabs->UpdateSize(Vector2(420.f, 220.f));
	root->AddNode(tabs);
	for (char const* name : { "First tab", "Second tab" })
	{
		auto* page = MemNew<Control>();
		page->SetName(name);
		tabs->AddPage(name, page);
	}
	auto* scroll = MemNew<ScrollContainer>();
	scroll->SetName("PreviewScroll");
	scroll->SetPosition(Vector2(500.f, 610.f));
	scroll->UpdateSize(Vector2(360.f, 220.f));
	root->AddNode(scroll);
	auto* list = MemNew<VBoxContainer>();
	scroll->AddNode(list);
	for (int i = 0; i < 16; ++i)
	{
		auto* entry = MemNew<Button>();
		entry->SetText(Stringf("Clipped rounded row %d", i + 1));
		list->AddNode(entry);
	}
	for (int i = 0; i < 3; ++i)
	{
		auto* progress = MemNew<ProgressBar>();
		progress->SetPosition(Vector2(930.f, 610.f + i * 60.f));
		progress->UpdateSize(Vector2(300.f, 24.f));
		progress->SetValue(i == 0 ? 1.f : i == 1 ? 50.f : 100.f);
		root->AddNode(progress);
	}

	float const layoutScale = std::max(1.f, scale / 1.25f);
	for (Node* child : root->GetChildren())
	{
		if (auto* control = dynamic_cast<Control*>(child))
		{
			control->SetPosition(control->GetLocalPosition() * layoutScale);
			control->UpdateSize(control->GetSize() * layoutScale);
		}
	}
}
}

void VerifyUIFlow(Engine& engine, float layoutScale)
{
	auto*      viewport = static_cast<Viewport*>(engine.m_sceneSystem->GetSceneTree()->GetRoot());
	auto*      root     = static_cast<Control*>(viewport->GetNodeByString("RoundedThemePreview"));
	auto*      input    = static_cast<LineEdit*>(root->GetNodeByString("InputField"));
	auto*      scroll   = static_cast<ScrollContainer*>(root->GetNodeByString("PreviewScroll"));
	auto*      tabs     = static_cast<TabContainer*>(root->GetNodeByString("PreviewTabs"));
	InputEvent windowFocus;
	windowFocus.m_type    = InputEventType::WindowFocus;
	windowFocus.m_focused = true;
	viewport->ProcessInputEvent(windowFocus);
	auto click = [viewport](Vector2 point)
	{
		InputEvent event;
		event.m_type     = InputEventType::MouseMotion;
		event.m_position = point;
		viewport->ProcessInputEvent(event);
		event.m_type        = InputEventType::MouseButton;
		event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
		event.m_pressed     = true;
		viewport->ProcessInputEvent(event);
		event.m_pressed = false;
		viewport->ProcessInputEvent(event);
	};
	click(Vector2(330.f, 540.f) * layoutScale);
	Check(viewport->GetGuiFocus() == input, "rounded input accepts viewport focus");
	InputEvent text;
	text.m_type      = InputEventType::Text;
	text.m_codepoint = 'R';
	viewport->ProcessInputEvent(text);
	Check(input->GetText() == "R", "rounded input preserves text routing");
	input->SetText("");
	InputEvent wheel;
	wheel.m_type     = InputEventType::MouseWheel;
	wheel.m_position = Vector2(600.f, 710.f) * layoutScale;
	wheel.m_scroll   = Vector2(0.f, -1.f);
	viewport->ProcessInputEvent(wheel);
	Check(scroll->GetScroll() > 0.f, "rounded rows retain scroll input");
	scroll->SetScroll(0.f);
	auto* second = tabs->GetTabButton(1);
	click(second->GetGlobalPosition() + Vector2(10.f, 10.f));
	Check(tabs->GetCurrentTab() == 1, "rounded tab changes page through viewport input");
	click(Vector2(100.f, 540.f) * layoutScale);
	Check(viewport->GetGuiPopup() != nullptr, "rounded button opens dialog through viewport input");
	InputEvent escape;
	escape.m_type    = InputEventType::Key;
	escape.m_pressed = true;
	escape.m_keyCode = GLFW_KEY_ESCAPE;
	viewport->ProcessInputEvent(escape);
	Check(viewport->GetGuiPopup() == nullptr, "rounded dialog closes with Escape");
}

#include <d3d11.h>

void CaptureRenderedFrame(Engine& engine, std::string const& path, float layoutScale = 1.f)
{
	auto* viewport = static_cast<Viewport*>(engine.m_sceneSystem->GetSceneTree()->GetRoot());
	auto* texture  = engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
	Check(texture != nullptr, "rendered viewport texture exists");
	ID3D11Resource* resource = nullptr;
	texture->GetShaderResourceView()->GetResource(&resource);
	ID3D11Texture2D* source = nullptr;
	Check(
		SUCCEEDED(resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&source))),
		"readback texture");
	resource->Release();
	ID3D11Device* device = nullptr;
	source->GetDevice(&device);
	ID3D11DeviceContext* context = nullptr;
	device->GetImmediateContext(&context);
	D3D11_TEXTURE2D_DESC description;
	source->GetDesc(&description);
	description.Usage          = D3D11_USAGE_STAGING;
	description.BindFlags      = 0;
	description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	description.MiscFlags      = 0;
	ID3D11Texture2D* staging   = nullptr;
	Check(SUCCEEDED(device->CreateTexture2D(&description, nullptr, &staging)), "create readback staging texture");
	context->CopyResource(staging, source);
	D3D11_MAPPED_SUBRESOURCE mapped;
	Check(SUCCEEDED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)), "map rendered pixels");
	if (path.find("preview-") != std::string::npos)
	{
		auto pixel = [&mapped, layoutScale](int x, int y)
		{
			x                = static_cast<int>(x * layoutScale);
			y                = static_cast<int>(y * layoutScale);
			auto const* rgba = static_cast<unsigned char const*>(mapped.pData) + y * mapped.RowPitch + x * 4;
			return Color(NormalizeByte(rgba[0]), NormalizeByte(rgba[1]), NormalizeByte(rgba[2]), NormalizeByte(rgba[3]));
		};
		auto sameRGB = [](Color a, Color b)
		{
			return a.r == b.r && a.g == b.g && a.b == b.b;
		};
		Color const background = pixel(250, 95);
		Check(sameRGB(pixel(255, 100), background), "GPU leaves rounded button corner clear");
		Check(!sameRGB(pixel(400, 130), background), "GPU fills rounded button interior");
		Check(sameRGB(pixel(420, 450), background), "GPU focus outline leaves interior transparent");
		Check(sameRGB(pixel(600, 832), background), "GPU clips rounded scrolling rows to parent bounds");
	}
	std::ofstream output(path, std::ios::binary);
	output << "P6\n" << description.Width << ' ' << description.Height << "\n255\n";
	for (unsigned int y = 0; y < description.Height; ++y)
	{
		auto const* row = static_cast<unsigned char const*>(mapped.pData) + y * mapped.RowPitch;
		for (unsigned int x = 0; x < description.Width; ++x)
		{
			output.write(reinterpret_cast<char const*>(row + x * 4), 3);
		}
	}
	Check(output.good(), "write GPU frame evidence");
	context->Unmap(staging, 0);
	staging->Release();
	context->Release();
	device->Release();
	source->Release();
}

Node* FindNamed(Node* node, String const& name)
{
	if (node->IsQueuedForDeletion())
	{
		return nullptr;
	}

	if (node->GetName() == name)
	{
		return node;
	}

	for (Node* child : node->GetChildren())
	{
		if (Node* found = FindNamed(child, name))
		{
			return found;
		}
	}

	return nullptr;
}

Button* FindTextButton(Node* node, String const& text)
{
	if (auto* button = dynamic_cast<Button*>(node); button != nullptr && button->GetText() == text)
	{
		return button;
	}

	for (Node* child : node->GetChildren())
	{
		if (Button* found = FindTextButton(child, text))
		{
			return found;
		}
	}

	return nullptr;
}

void Press(Button* button)
{
	Check(button != nullptr && button->IsVisibleInHierarchy() && !button->IsDisabled(), "action available");
	button->GrabFocus();
	InputEvent event;
	event.m_type = InputEventType::MouseButton;
	event.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
	event.m_position = button->GetSize() * 0.5f;
	event.m_pressed = true;
	button->GuiInput(event);
	event.m_pressed = false;
	button->GuiInput(event);
}

void ReplaceText(LineEdit* edit, std::string const& text)
{
	InputEvent focus;
	focus.m_type = InputEventType::WindowFocus;
	focus.m_focused = true;
	edit->GetViewport()->ProcessInputEvent(focus);
	edit->GrabFocus();
	edit->SelectAll();
	InputEvent key;
	key.m_type = InputEventType::Key;
	key.m_keyCode = GLFW_KEY_BACKSPACE;
	key.m_pressed = true;
	edit->GuiInput(key);
	for (char character : text)
	{
		InputEvent event;
		event.m_type = InputEventType::Text;
		event.m_codepoint = static_cast<unsigned char>(character);
		edit->GuiInput(event);
	}
}

void VerifyProjectSettings(App& app, EditorUIContext& context, float scale, std::filesystem::path const& directory)
{
	ProjectSettingsPopup settings;
	auto frames = [&]()
	{
		settings.Render(context);
		for (int frame = 0; frame < 3; ++frame)
		{
			app.RunFrame();
		}
	};
	auto find = [&](char const* name)
	{
		Node* result = FindNamed(settings.GetContentControl(), name);
		Check(result != nullptr, "settings control available");
		return result;
	};
	auto capture = [&](std::string const& name)
	{
		CaptureRenderedFrame(*app.GetEngine(), "../Temporary/RoundedThemeVerification/" + name + "-"
			+ std::to_string(static_cast<int>(scale * 100)) + ".ppm");
	};
	auto browse = [&](char const* action, char const* filename, char const* confirm)
	{
		Press(dynamic_cast<Button*>(find(action)));
		app.RunFrame();
		auto* file = dynamic_cast<LineEdit*>(FindNamed(context.m_sceneTree->GetRoot(), "SceneFilename"));
		Check(file != nullptr && file->IsVisibleInHierarchy(), "file picker opens from settings");
		file->SetText(filename);
		Press(FindTextButton(file->GetParent(), confirm));
		frames();
		Check(settings.IsOpen(), "settings returns from file picker");
	};

	settings.Open();
	frames();
	auto* tabs = dynamic_cast<TabContainer*>(find("ProjectSettingsTabs"));
	Check(tabs->GetPageCount() == 2, "general and globals navigation retained");
	capture("settings");
	Press(tabs->GetTabButton(1));
	frames();
	capture("autoload");
	Press(tabs->GetTabButton(0));
	frames();
	auto* filter = dynamic_cast<LineEdit*>(find("FilterSettings"));
	auto* fields = dynamic_cast<Control*>(find("RunSettings"));
	ReplaceText(filter, "unmatched");
	Check(!fields->IsVisible(), "settings filter hides unmatched content");
	ReplaceText(filter, "main");
	Check(fields->IsVisible(), "settings filter restores matching content");
	ReplaceText(filter, "");
	auto* mainScene = dynamic_cast<LineEdit*>(find("ProjectMainScene"));
	mainScene->SetText("res://missing.tscn");
	Check(!settings.Confirm() && !settings.GetError().empty(), "invalid main scene cannot be saved");
	browse("BrowseMainScene", "main.tscn", "Select Main Scene");
	Check(mainScene->GetText() == String("res://main.tscn"), "main scene browse updates draft");

	Press(tabs->GetTabButton(1));
	frames();
	auto* empty = dynamic_cast<Control*>(find("AutoloadEmptyState"));
	Check(empty->IsVisibleInHierarchy(), "autoload empty state visible");
	browse("BrowseAutoload", "Session.cs", "Select Autoload");
	auto* newName = dynamic_cast<LineEdit*>(find("NewAutoloadName"));
	auto* newPath = dynamic_cast<LineEdit*>(find("NewAutoloadPath"));
	Check(newName->GetText() == String("Session"), "autoload browse derives node name");
	Press(dynamic_cast<Button*>(find("AddAutoload")));
	frames();
	Check(!empty->IsVisibleInHierarchy() && newPath->GetText().IsEmpty(), "add replaces empty state and clears input");
	newName->SetText("Bootstrap");
	newPath->SetText("res://main.tscn");
	Press(dynamic_cast<Button*>(find("AddAutoload")));
	frames();
	auto* rows = find("AutoloadEntries");
	auto* firstName = dynamic_cast<LineEdit*>(find("AutoloadName_0"));
	ReplaceText(firstName, "SessionManager");
	auto* firstRow = firstName->GetParent();
	Check(dynamic_cast<Button*>(FindNamed(firstRow, "MoveAutoloadUp"))->IsDisabled(), "first row cannot move up");
	Press(dynamic_cast<Button*>(FindNamed(firstRow, "MoveAutoloadDown")));
	frames();
	Check(dynamic_cast<LineEdit*>(find("AutoloadName_1"))->GetText() == String("SessionManager"),
		"edited name follows reordered entry");
	auto* secondRow = find("AutoloadName_1")->GetParent();
	Check(dynamic_cast<Button*>(FindNamed(secondRow, "MoveAutoloadDown"))->IsDisabled(), "last row cannot move down");
	Press(dynamic_cast<Button*>(FindNamed(secondRow, "MoveAutoloadUp")));
	frames();
	secondRow = find("AutoloadName_1")->GetParent();
	Press(dynamic_cast<Button*>(FindNamed(secondRow, "RemoveAutoload")));
	frames();
	Check(FindNamed(rows, "AutoloadName_1") == nullptr, "remove deletes selected entry");
	Press(dynamic_cast<Button*>(find("SaveProjectSettings")));
	Check(!settings.IsOpen() && ProjectSettings::Get()->m_autoLoads.size() == 1
		&& ProjectSettings::Get()->m_autoLoads.front().m_name == "SessionManager", "save commits edited autoload");
	std::string error;
	auto saved = ProjectSettings::Read(directory, error);
	Check(saved.IsValid() && saved->m_autoLoads.size() == 1
		&& saved->m_autoLoads.front().m_name == "SessionManager", "saved settings round trip");

	settings.Open();
	frames();
	ReplaceText(dynamic_cast<LineEdit*>(find("AutoloadName_0")), "UnsavedName");
	browse("BrowseAutoload", "", "Cancel");
	Check(dynamic_cast<LineEdit*>(find("AutoloadName_0"))->GetText() == String("UnsavedName"),
		"file picker cancel preserves draft");
	Press(dynamic_cast<Button*>(find("CancelProjectSettings")));
	Check(!settings.IsOpen() && ProjectSettings::Get()->m_autoLoads.front().m_name == "SessionManager",
		"cancel discards unsaved rename");
	settings.Open();
	frames();
	Press(dynamic_cast<Button*>(FindNamed(find("AutoloadName_0")->GetParent(), "RemoveAutoload")));
	frames();
	Check(empty->IsVisibleInHierarchy(), "removing final entry restores empty state");
	settings.Cancel();

	auto settingsData = ProjectSettings::Get();
	settingsData->m_autoLoads.clear();
	for (int index = 0; index < 18; ++index)
	{
		settingsData->m_autoLoads.push_back({ "Service" + std::to_string(index), VirtualPath("res://Session.cs") });
	}

	settings.Open();
	frames();
	auto* list = dynamic_cast<ScrollContainer*>(find("AutoloadList"));
	auto* path = dynamic_cast<Control*>(find("AutoloadPath_0"));
	auto* pathHeader = dynamic_cast<Control*>(find("AutoloadPathHeader"));
	auto* actionsHeader = dynamic_cast<Control*>(find("AutoloadActionsHeader"));
	auto* up = dynamic_cast<Control*>(FindNamed(find("AutoloadName_0")->GetParent(), "MoveAutoloadUp"));
	Check(Near(path->GetGlobalPosition().x, pathHeader->GetGlobalPosition().x)
		&& Near(up->GetGlobalPosition().x, actionsHeader->GetGlobalPosition().x), "table columns align with headers");
	capture("autoload-populated");
	list->EnsureControlVisible(dynamic_cast<Control*>(find("AutoloadName_17")));
	Check(list->GetScroll() > 0.f, "long autoload list scrolls to last entry");

	glfwSetWindowSize(app.GetEngine()->m_windowSystem->GetGLFWWindow(), 1100, 760);
	frames();
	frames();
	list->SetScroll(0.f);
	frames();
	capture("autoload-small");
	Check(Near(path->GetGlobalPosition().x, pathHeader->GetGlobalPosition().x), "columns remain aligned after resize");
	Press(tabs->GetTabButton(0));
	frames();
	capture("settings-small");
	settings.Cancel();
}

void VerifyEditorFrames(float scale)
{
	auto const directory = std::filesystem::absolute("../Temporary/RoundedThemeVerification/EditorProject");
	std::filesystem::create_directories(directory / "Settings");
	_wputenv_s(L"LOCALAPPDATA", (directory / "Settings").c_str());
	{
		std::ofstream project(directory / "project.ming");
		project << R"({"type":"ProjectSettings","version":1,"start_scene_path":"res://main.tscn"})";
		std::ofstream scene(directory / "main.tscn");
		scene
			<< R"([ming_scene format=1]

[node name="Main" type="Node" parent=""]

[node name="SampleButton" type="Button" parent="."]
)";
		std::ofstream script(directory / "Session.cs");
		script << "public class Session : Ming.Node {}";
	}
	MingRunConfig config;
	config.mode        = MingRunMode::Editor;
	config.projectPath = directory;
	App app(config);
	app.Startup();
	Check(!app.IsQuitting(), "editor startup");
	glfwHideWindow(app.GetEngine()->m_windowSystem->GetGLFWWindow());
	auto* editor = EditorNode::Get();
	Check(editor != nullptr && editor->SetEditorScale(scale), "editor scale applies");
	app.RunFrame();
	app.RunFrame();
	auto* scene = app.GetEngine()->m_sceneSystem->GetCurrentScene();
	Check(scene != nullptr, "editor fixture scene loaded");
	auto* button = scene->GetNodeByString("SampleButton");
	Check(button != nullptr, "editor fixture control loaded");
	editor->GetSelection().SetSelected(button->GetObjectID());
	for (int frame = 0; frame < 5; ++frame)
	{
		app.RunFrame();
	}
	std::string const suffix = std::to_string(static_cast<int>(scale * 100));
	CaptureRenderedFrame(*app.GetEngine(), "../Temporary/RoundedThemeVerification/editor-" + suffix + ".ppm");
	editor->m_editorUI->ShowMenu(
		editor->m_editorUI->GetRootControl(),
		Vector2(200.f, 100.f),
		{ { "Rounded menu item" }, { "Second item" } });
	for (int frame = 0; frame < 3; ++frame)
	{
		app.RunFrame();
	}
	CaptureRenderedFrame(*app.GetEngine(), "../Temporary/RoundedThemeVerification/menu-" + suffix + ".ppm");
	EditorUIContext      context;
	context.m_rootControl = editor->m_editorUI->GetRootControl();
	context.m_sceneTree   = app.GetEngine()->m_sceneSystem->GetSceneTree();
	context.m_editorUI    = editor->m_editorUI;
	context.m_fileSystem  = app.GetEngine()->m_fileSystem;
	VerifyProjectSettings(app, context, scale, directory);
	app.Shutdown();
}

int main(int argc, char** argv)
{
	try
	{
		bool const  preview = argc > 1 && std::string(argv[1]) == "--preview";
		bool const  capture = argc > 1 && std::string(argv[1]) == "--capture";
		float const scale   = argc > 2 ? std::stof(argv[2]) : 1.25f;
		if (argc > 1 && std::string(argv[1]) == "--editor-capture")
		{
			VerifyEditorFrames(scale);
			std::printf("ROUNDED_EDITOR checks=%d failures=0\n", checks);
			return 0;
		}
		ClassDatabase::Startup();
		RegisterAllTypes();
		EngineConfig config;
		config.m_windowConfig.m_appName         = "Ming Rounded Theme Verification";
		config.m_windowConfig.m_isResizable     = true;
		config.m_scriptSystemConfig.m_isEnabled = false;
		config.m_fileSystemConfig.m_isEnable    = false;
		config.m_audioConfig.m_isEnable         = false;
		config.m_networkConfig.m_isEnable       = false;
		{
			Engine engine(config);
			Check(engine.Startup(), "verification engine startup");
			if (!preview)
			{
				glfwHideWindow(engine.m_windowSystem->GetGLFWWindow());
			}
			VerifyResources();
			VerifyGeometry();
			VerifyStandards();
			PopulatePreview(engine, scale);
			int frame = 0;
			while (!glfwWindowShouldClose(engine.m_windowSystem->GetGLFWWindow()) && (preview || frame < 4))
			{
				engine.BeginFrame();
				engine.Update(1.f / 60.f);
				engine.Render();
				engine.EndFrame();
				++frame;
			}
			if (!preview)
			{
				VerifyUIFlow(engine, std::max(1.f, scale / 1.25f));
			}
			if (capture)
			{
				std::string const suffix = std::to_string(static_cast<int>(scale * 100));
				CaptureRenderedFrame(
					engine,
					"../Temporary/RoundedThemeVerification/preview-" + suffix + ".ppm",
					std::max(1.f, scale / 1.25f));
				auto*          viewport = static_cast<Viewport*>(engine.m_sceneSystem->GetSceneTree()->GetRoot());
				auto*          owner    = static_cast<Control*>(viewport->GetNodeByString("RoundedThemePreview"));
				VBoxContainer* content  = nullptr;
				auto*          popup    = EditorPopupUtils::Create(owner, "Rounded dialog", content, true);
				EditorPopupUtils::AddLabel(content, "Header and body preserve outer corners.");
				auto* input = MemNew<LineEdit>();
				input->SetPlaceholder("Input field");
				content->AddNode(input);
				popup->Open(owner);
				EditorPopupUtils::Layout(popup, Vector2(560.f, 300.f));
				for (int i = 0; i < 3; ++i)
				{
					engine.BeginFrame();
					engine.Update(1.f / 60.f);
					engine.Render();
					engine.EndFrame();
				}
				CaptureRenderedFrame(engine, "../Temporary/RoundedThemeVerification/dialog-" + suffix + ".ppm");
			}
			engine.m_sceneSystem->Shutdown();
			ThemeDatabase::Shutdown();
			engine.Shutdown();
		}
		g_engine = nullptr;
		ClassDatabase::Shutdown();
		std::printf("ROUNDED_THEME checks=%d failures=0\n", checks);
		return 0;
	}
	catch (std::exception const& error)
	{
		std::fprintf(stderr, "ROUNDED_THEME failure: %s\n", error.what());
		return 1;
	}
}
