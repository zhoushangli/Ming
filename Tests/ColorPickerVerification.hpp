#pragma once

#include "MingEngine/Scene/GUI/ColorPickerButton.hpp"
#include "MingEngine/Scene/GUI/SpinBox.hpp"
#include "MingEngine/Scene/GUI/ScrollContainer.hpp"
#include "MingEngine/Scene/GUI/PopupPanel.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"
#include "ThirdParty/GLFW/glfw3.h"

namespace ColorPickerVerification
{
template <typename T>
T* FindColorControl(Node* node, String const& name)
{
	if (auto* result = dynamic_cast<T*>(node); result != nullptr && node->GetName() == name)
	{
		return result;
	}

	for (Node* child : node->GetChildren())
	{
		if (T* result = FindColorControl<T>(child, name))
		{
			return result;
		}
	}

	return nullptr;
}

inline void SendColorKey(Viewport* root, int code)
{
	InputEvent event;
	event.m_type    = InputEventType::Key;
	event.m_keyCode = code;
	event.m_pressed = true;
	root->ProcessInputEvent(event);
	event.m_pressed = false;
	root->ProcessInputEvent(event);
}

inline void DraftColorText(Viewport* root, LineEdit* edit, std::string const& text)
{
	root->SetGuiFocus(nullptr);
	edit->GrabFocus();
	edit->SelectAll();
	SendColorKey(root, GLFW_KEY_BACKSPACE);
	for (char c : text)
	{
		InputEvent event;
		event.m_type      = InputEventType::Text;
		event.m_codepoint = static_cast<uint32_t>(c);
		root->ProcessInputEvent(event);
	}
}

inline void EnterHex(Viewport* root, ColorPicker* picker, char const* text)
{
	DraftColorText(root, FindColorControl<LineEdit>(picker, "HexColor"), text);
	SendColorKey(root, GLFW_KEY_ENTER);
}

template <typename Check>
void Run(Check&& check)
{
	SceneTree tree;
	auto*     root = static_cast<Viewport*>(tree.GetRoot());
	root->UpdateResolution(IntVec2(1280, 900));
	auto* owner = MemNew<Control>();
	root->AddNode(owner);
	auto* button = MemNew<ColorPickerButton>();
	owner->AddNode(button);
	button->SetPosition(Vector2(20.f, 20.f));
	button->UpdateSize(Vector2(160.f, 32.f));
	button->SetColorNoSignal(Color(10, 20, 30, 40));
	int  starts   = 0;
	int  ends     = 0;
	bool accepted = false;
	button->SetEditingStartedCallback(
		[&]()
		{
			++starts;
		});
	button->SetEditingEndedCallback(
		[&](bool accept)
		{
			++ends;
			accepted = accept;
		});
	auto pointer =
		[&](InputEventType type, Vector2 position, bool pressed = false, int mouseButton = GLFW_MOUSE_BUTTON_LEFT)
	{
		InputEvent event;
		event.m_type        = type;
		event.m_position    = position;
		event.m_pressed     = pressed;
		event.m_mouseButton = mouseButton;
		root->ProcessInputEvent(event);
	};
	button->GrabFocus();
	SendColorKey(root, GLFW_KEY_ENTER);
	ColorPicker* picker = button->GetPicker();
	check(starts == 1 && button->IsPickerOpen(), "color button opens from keyboard");
	DraftColorText(root, FindColorControl<SpinBox>(picker, "Alpha")->GetLineEdit(), "255");
	SendColorKey(root, GLFW_KEY_ENTER);
	check(button->GetColor() == Color(10, 20, 30, 255), "color channel preserves other bytes");
	EnterHex(root, picker, "ff800080");
	check(button->GetColor() == Color(255, 128, 0, 128), "eight digit Hex edits RGBA");
	EnterHex(root, picker, "#00ff00");
	check(button->GetColor() == Color(0, 255, 0, 255), "six digit Hex is opaque");
	EnterHex(root, picker, "#bad-input");
	check(
		button->GetColor() == Color(0, 255, 0)
			&& FindColorControl<LineEdit>(picker, "HexColor")->GetText() == String("#00FF00FF"),
		"invalid Hex restores canonical value");
	DraftColorText(root, FindColorControl<LineEdit>(picker, "HexColor"), "#1234");
	picker->SetColorNoSignal(picker->GetColor());
	check(
		FindColorControl<LineEdit>(picker, "HexColor")->GetText() == String("#1234"),
		"unchanged refresh preserves Hex draft");
	SendColorKey(root, GLFW_KEY_ESCAPE);
	check(button->IsPickerOpen() && button->GetColor() == Color(0, 255, 0), "first Escape cancels only draft");
	SendColorKey(root, GLFW_KEY_ESCAPE);
	check(
		!button->IsPickerOpen() && !accepted && button->GetColor() == Color(10, 20, 30, 40),
		"second Escape cancels entire color edit");
	button->OpenPicker();
	picker->SetHSVMode(true);
	auto channel = [&](int index, char const* value)
	{
		DraftColorText(
			root,
			FindColorControl<SpinBox>(picker, String("Channel" + std::to_string(index)))->GetLineEdit(),
			value);
		SendColorKey(root, GLFW_KEY_ENTER);
	};
	channel(0, "240");
	channel(1, "0");
	channel(2, "100");
	channel(1, "100");
	check(button->GetColor() == Color(0, 0, 255, 40), "HSV retains hue through gray");
	channel(2, "0");
	channel(0, "120");
	channel(2, "100");
	check(button->GetColor() == Color(0, 255, 0, 40), "HSV retains edited hue and saturation through black");
	channel(0, "0");
	channel(2, "50");
	check(button->GetColor() == Color(128, 0, 0, 40), "HSV conversion rounds half intensity to nearest byte");
	Color const beforeMode = button->GetColor();
	picker->SetHSVMode(false);
	check(button->GetColor() == beforeMode, "mode switch does not quantize or publish color");
	channel(0, "256");
	check(button->GetColor() == beforeMode, "out of range numeric draft restores value");
	Vector2 const start = picker->GetGlobalPosition() + picker->GetSelectionRect().GetCenter();
	pointer(InputEventType::MouseButton, start, true);
	pointer(
		InputEventType::MouseMotion,
		picker->GetGlobalPosition() + picker->GetSelectionRect().m_maxs + Vector2(30.f, 30.f));
	pointer(InputEventType::MouseButton, start);
	check(button->GetColor() == Color(0, 0, 0, 40), "selection drag clamps outside its rectangle");
	EnterHex(root, picker, "#789abc80");
	pointer(InputEventType::MouseButton, Vector2(1000.f, 800.f), true);
	pointer(InputEventType::MouseButton, Vector2(1000.f, 800.f));
	check(
		accepted && ends == 2 && !button->IsPickerOpen() && button->GetColor() == Color(120, 154, 188, 128),
		"outside click commits once");
	int   underlyingClicks = 0;
	auto* underlying       = MemNew<Button>();
	root->AddNode(underlying);
	underlying->SetPosition(Vector2(800.f, 20.f));
	underlying->UpdateSize(Vector2(120.f, 40.f));
	underlying->SetPressedCallback(
		[&]()
		{
			++underlyingClicks;
		});
	bool samplingWorks = true;
	button->SetScreenColorReader(
		[&](Vector2 const&, Color& sample)
		{
			sample = Color(12, 34, 56, 3);
			return samplingWorks;
		});
	button->OpenPicker();
	button->StartScreenPicking();
	pointer(InputEventType::MouseMotion, Vector2(850.f, 40.f));
	check(button->IsPickingScreen() && button->GetColor() == Color(12, 34, 56), "screen preview forces opaque Alpha");
	samplingWorks = false;
	pointer(InputEventType::MouseButton, Vector2(850.f, 40.f), true);
	pointer(InputEventType::MouseButton, Vector2(850.f, 40.f));
	check(
		button->IsPickingScreen() && button->GetColor() == Color(12, 34, 56),
		"failed sample cannot confirm or overwrite preview");
	pointer(InputEventType::MouseMotion, Vector2(-5.f, 40.f));
	check(button->IsPickingScreen(), "leaving window pauses screen picking");
	SendColorKey(root, GLFW_KEY_ESCAPE);
	check(
		!button->IsPickingScreen() && button->IsPickerOpen() && button->GetColor() == Color(120, 154, 188, 128),
		"Escape in eyedropper restores pre-pick color and returns to picker");
	samplingWorks = true;
	button->StartScreenPicking();
	pointer(InputEventType::MouseButton, Vector2(850.f, 40.f), true);
	pointer(InputEventType::MouseButton, Vector2(850.f, 40.f));
	check(
		!button->IsPickingScreen() && button->IsPickerOpen() && underlyingClicks == 0,
		"sample confirmation returns to picker without clicking underlying button");
	InputEvent focus;
	focus.m_type    = InputEventType::WindowFocus;
	focus.m_focused = false;
	root->ProcessInputEvent(focus);
	check(
		!button->IsPickerOpen() && !accepted && button->GetColor() == Color(120, 154, 188, 128),
		"window focus loss rolls back the full color edit");
	focus.m_focused = true;
	root->ProcessInputEvent(focus);
	button->SetPosition(Vector2(1200.f, 850.f));
	button->OpenPicker();
	Vector2 const position = picker->GetGlobalPosition();
	check(
		position.x >= 0.f && position.y >= 0.f && position.x + picker->GetSize().x <= 1280.f
			&& position.y + picker->GetSize().y <= 900.f,
		"color popup stays inside bottom-right window edge");
	owner->SetVisible(false);
	tree.UpdateScene(1.f / 60.f);
	check(!button->IsPickerOpen() && !accepted, "hiding an ancestor cancels its detached color popup");
	owner->SetVisible(true);
	button->OpenPicker();
	root->UpdateResolution(IntVec2(240, 240));
	tree.UpdateScene(1.f / 60.f);
	auto* scroll = static_cast<ScrollContainer*>(picker->GetParent());
	check(
		scroll->GetGlobalPosition() == Vector2::Zero && scroll->GetSize() == Vector2(240.f, 240.f)
			&& scroll->GetPageSize().y < picker->GetSize().y,
		"resizing to a small window keeps picker accessible by scrolling");
	button->QueueFree();
	tree.FlushPendingNode();
	check(root->GetGuiPopup() == nullptr, "deleting color button clears popup and callbacks");
}
}
