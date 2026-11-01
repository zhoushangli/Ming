#pragma once

#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"
#include "MingEngine/Scene/GUI/OptionButton.hpp"

#include <functional>
#include <cstdlib>
#include <thread>
#include <chrono>

class ThemeProbe : public Button
{
	MCLASS(ThemeProbe, Button);

private:
	struct ThemeCache
	{
		int spacing = 0;
	};

public:
	int         GetSpacing() const { return m_themeCache.spacing; }
	int         GetRefreshCount() const { return m_refreshCount; }
	static void BindMethods() { BIND_THEME_ITEM(Theme::DataType::Constant, ThemeProbe, spacing); }

protected:
	void OnThemeChanged() override
	{
		Button::OnThemeChanged();
		++m_refreshCount;
	}

private:
	ThemeCache m_themeCache;
	int        m_refreshCount = 0;
};

inline Ref<Theme> CreateWarmCarbonInkTheme(Ref<FontResource> const& font)
{
	auto theme = CreateRef<Theme>();
	theme->SetFont("font", "Control", font);
	theme->SetFontSize("font_size", "Control", 12);
	theme->SetColor("font_color", "Control", Color(222, 217, 208));
	char const* colors[] = { "font_color",
							 "font_hover_color",
							 "font_pressed_color",
							 "font_hover_pressed_color",
							 "font_disabled_color" };
	for (auto key : colors)
	{
		theme->SetColor(key, "Button", Color(222, 217, 208));
	}

	theme->SetColor("font_disabled_color", "Button", Color(127, 121, 110));
	char const* states[]      = { "normal", "hover", "pressed", "hover_pressed", "disabled", "focus" };
	Color const backgrounds[] = { Color::Transparent, Color(44, 42, 39),  Color(29, 28, 26),
								  Color(29, 28, 26),  Color::Transparent, Color::Transparent };
	for (int i = 0; i < 6; ++i)
	{
		auto box = CreateRef<ThemeStyleBox>();
		box->SetBackgroundColor(backgrounds[i]);
		box->SetBorderColor(i == 5 ? Color(211, 173, 114) : Color(81, 74, 64));
		for (int side = 0; side < 4; ++side)
		{
			box->SetBorderWidth(static_cast<ThemeStyleBox::Side>(side), i == 2 || i == 3 || i == 5 ? 1.f : 0.f);
			box->SetContentMargin(static_cast<ThemeStyleBox::Side>(side), i == 5 ? 0.f : (side % 2 == 0 ? 8.f : 4.f));
		}

		theme->SetStyleBox(states[i], "Button", box);
	}

	return theme;
}

inline void VerifyThemeContexts(std::function<void(bool, char const*)> const& check)
{
	SceneTree tree;
	auto* scope = MemNew<Control>();
	tree.GetRoot()->AddNode(scope);
	auto first = CreateRef<Theme>();
	auto second = CreateRef<Theme>();
	first->SetDefaultFontSize(13);
	first->SetDefaultFont(ThemeDatabase::GetDefaultTheme()->GetDefaultFont());
	second->SetFontSize("font_size", "Button", 17);
	ThemeDatabase::SetThemeContext(scope, { first, second });
	auto* button = MemNew<Button>();
	button->SetText("Context");
	scope->AddNode(button);
	check(button->GetFontSize() == 13 && button->GetFont() == first->GetDefaultFont(),
		"context resolves default font and size from its first source");
	first->SetDefaultFontSize(-1);
	check(button->GetFontSize() == 17, "context missing item falls through to its next source");
	first->SetDefaultFontSize(13);
	first->SetFontSize("font_size", "BaseButton", 15);
	check(button->GetFontSize() == 15, "native base type item precedes same Theme default");
	first->RemoveFontSize("font_size", "BaseButton");
	auto local = CreateRef<Theme>();
	local->SetDefaultFontSize(21);
	button->SetTheme(local);
	button->SetFontSize(25);
	check(button->GetFontSize() == 25, "local override precedes local Theme and Context");
	button->RemoveThemeFontSizeOverride("font_size");
	check(button->GetFontSize() == 21, "local Theme precedes Context");
	button->SetTheme({});
	auto* inner = MemNew<Node>();
	scope->AddNode(inner);
	ThemeDatabase::SetThemeContext(inner, {});
	button->Reparent(inner);
	check(button->GetFontSize() == ThemeDatabase::GetDefaultTheme()->GetDefaultFontSize(),
		"nearest empty Context isolates outer editor defaults");
	ThemeDatabase::ClearThemeContext(inner);
	check(button->GetFontSize() == 13, "removing Context refreshes descendants and restores outer Context");

	auto* popup = MemNew<PopupPanel>();
	tree.GetRoot()->AddNode(popup);
	auto* option = MemNew<Button>();
	popup->AddNode(option);
	popup->Open(button);
	check(option->GetFontSize() == 13, "viewport popup resolves its owner's Context");
	first->SetDefaultFontSize(14);
	check(option->GetFontSize() == 14, "popup observes in-place Context Theme changes");
	button->SetTheme(local);
	check(option->GetFontSize() == 21, "owner Theme replacement refreshes detached popup content");
	button->SetFontSize(27);
	button->SetThemeTypeVariation("UnrelatedOwnerVariation");
	check(option->GetFontSize() == 21, "popup does not inherit owner overrides or type variation");
	auto popupTheme = CreateRef<Theme>();
	popupTheme->SetDefaultFontSize(19);
	popup->SetTheme(popupTheme);
	check(option->GetFontSize() == 19, "popup local Theme precedes owner sources");
	check(!button->SetThemeSource(option), "theme source cycle is rejected");
	button->QueueFree();
	tree.FlushPendingNode();
	check(!popup->IsVisible() && popup->GetThemeSource() == nullptr, "owner deletion closes popup and releases weak source");

	auto* replacement = MemNew<Button>();
	scope->AddNode(replacement);
	popup->SetTheme({});
	popup->Open(replacement);
	check(option->GetFontSize() == 14, "popup reopening resolves a new valid owner");
	ThemeDatabase::SetThemeContext(scope, { second });
	check(option->GetFontSize() == 17, "Context replacement refreshes detached popup");
	auto moved = CreateRef<Theme>();
	moved->SetDefaultFont(first->GetDefaultFont());
	moved->SetDefaultFontSize(22);
	check(second->CopyFrom(std::move(*moved)) && replacement->GetFontSize() == 22
		&& option->GetFontSize() == 22, "Theme CopyFrom moves defaults and refreshes Context consumers");

	replacement->SetText("Icon");
	Vector2 const textSize = replacement->GetCombinedMinimumSize();
	auto icon = CreateRef<TextureResource>();
	icon->m_image = CreateRef<Image>();
	icon->m_image->LoadFromRGBA8(IntVec2(8, 8), std::vector<uint8_t>(8 * 8 * 4, 255));
	replacement->SetIcon(icon);
	check(replacement->GetChildren().empty() && replacement->GetCombinedMinimumSize().x > textSize.x,
		"Button text and icon contribute directly to minimum size without an internal Label");
	replacement->SetText("");
	check(replacement->GetCombinedMinimumSize().x > 0.f, "icon-only Button retains a nonempty minimum size");
}

inline void VerifyThemes(
	Engine& engine, std::function<void(bool, char const*)> const& check, std::filesystem::path const& projectPath)
{
	ClassDatabase::RegisterClass<ThemeProbe>();
	VerifyThemeContexts(check);
	auto font            = FontResource::CreateDefault();
	auto originalDefault = ThemeDatabase::GetDefaultTheme();
	int const defaultFontSize = originalDefault->GetDefaultFontSize();
	{
		SceneTree editTree;
		auto* edit = MemNew<LineEdit>();
		auto editTheme = CreateRef<Theme>();
		editTheme->SetFont("font", "LineEdit", font);
		auto normal = CreateRef<ThemeStyleBox>();
		normal->SetContentMargin(ThemeStyleBox::Side::Left, 11.f);
		normal->SetContentMargin(ThemeStyleBox::Side::Right, 7.f);
		normal->SetContentMargin(ThemeStyleBox::Side::Bottom, 5.f);
		editTheme->SetStyleBox("normal", "LineEdit", normal);
		edit->SetTheme(editTheme);
		editTree.GetRoot()->AddNode(edit);
		edit->UpdateSize(Vector2(140.f, 24.f));
		edit->SetText("A long editable field that scrolls");
		edit->GrabFocus();
		edit->Select(3, 8);
		int ended = 0;
		edit->SetEditingEndedCallback([&](String const&, LineEdit::EditEndReason) { ++ended; });
		bool const editing = edit->IsEditing();
		bool const undo = edit->HasUndo();
		check(edit->GetFont() == font && edit->GetFontSize() == defaultFontSize
			&& edit->GetPadding() == Vector2(11.f, 5.f), "LineEdit resolves font and asymmetric state margins");
		editTheme->SetFontSize("font_size", "LineEdit", 18);
		normal->SetContentMargin(ThemeStyleBox::Side::Left, 15.f);
		check(edit->GetCursorColumn() == 8 && edit->GetSelectionFromColumn() == 3
			&& edit->GetSelectionToColumn() == 8 && edit->IsEditing() == editing
			&& edit->HasUndo() == undo && ended == 0, "LineEdit theme reflow preserves active edit and selection");
		edit->SetPadding(Vector2(2.5f, 1.25f));
		normal->SetContentMargin(ThemeStyleBox::Side::Left, 17.f);
		check(edit->GetPadding() == Vector2(2.5f, 1.25f), "LineEdit explicit padding preserves fractional precision");
		edit->RemovePaddingOverride();
		check(edit->GetPadding() == Vector2(17.f, 5.f), "LineEdit removing padding override restores live theme margins");
		edit->SetFontSize(14);
		editTheme->SetFontSize("font_size", "LineEdit", 20);
		check(edit->GetFontSize() == 14, "LineEdit font setter remains an explicit theme override");
		edit->RemoveThemeFontSizeOverride("font_size");
		check(edit->GetFontSize() == 20, "LineEdit removing font size override restores theme");
		edit->SetEditingEndedCallback({});
	}

	{
		SceneTree rowTree;
		auto* rows = MemNew<Tree>();
		auto rowTheme = CreateRef<Theme>();
		rowTheme->SetFont("font", "Tree", font);
		rows->SetTheme(rowTheme);
		rowTree.GetRoot()->AddNode(rows);
		rows->UpdateSize(Vector2(260.f, 160.f));
		rows->SetItems({ { 1, 0, "Root", true, true }, { 2, 1, "Child", true, true }, { 3, 0, "Other", true, true } });
		rowTree.FlushPendingNode();
		check(rows->GetRowHeight() == 24.f && rows->GetItemRect(2).m_mins.y == 24.f
			&& rows->GetItemAtPosition(Vector2(100.f, 24.f)) == 2, "Tree compact row boundaries share draw and hit geometry");
		rows->Select(2);
		rows->EditSelected();
		auto* rename = dynamic_cast<LineEdit*>(rows->GetChildren()[0]);
		rename->Select(1, 3);
		int renamed = 0;
		rows->SetRenamedCallback([&](uint64_t, String const&) { ++renamed; });
		size_t const children = rows->GetChildren().size();
		rowTheme->SetConstant("row_height", "Tree", 40);
		rowTheme->SetConstant("indent", "Tree", 22);
		rowTheme->SetFontSize("font_size", "Tree", 18);
		check(rows->GetRowHeight() == 40.f && rows->GetItemRect(2).m_mins.y == 40.f
			&& rows->GetItemAtPosition(Vector2(100.f, 79.f)) == 2
			&& rows->GetItemAtPosition(Vector2(100.f, 80.f)) == 3, "Tree expanded row boundaries update together");
		check(rows->GetChildren().size() == children && rename->GetText() == String("Child")
			&& rename->GetCursorColumn() == 3 && rename->GetSelectionFromColumn() == 1
			&& rename->IsEditing() && renamed == 0 && rename->GetLocalPosition().y == 40.f,
			"Tree theme reflow reuses rows and preserves active rename");
		rows->SetCollapsed(1, true);
		check(rows->IsCollapsed(1) && rows->GetItemRect(2) == AABB2::Zero,
			"Tree collapsed children have no visible item rectangle");
		rows->SetRenamedCallback({});
	}

	{
		SceneTree tabTree;
		auto* tabs = MemNew<TabContainer>();
		auto tabTheme = CreateRef<Theme>();
		tabTheme->SetFont("font", "TabContainer", font);
		tabs->SetTheme(tabTheme);
		tabTree.GetRoot()->AddNode(tabs);
		auto* first = MemNew<Control>();
		tabs->AddPage("One", first);
		tabs->AddPage("Two", MemNew<Control>());
		tabs->UpdateSize(Vector2(300.f, 180.f));
		tabTree.FlushContainerUpdate();
		check(tabs->GetTabBarRect().GetHeight() == 26.f && first->GetLocalPosition().y == 26.f
			&& tabs->GetTabInsertRect(1).GetHeight() == 26.f, "Tab compact page and insertion geometry use header height");
		auto selected = CreateRef<ThemeStyleBox>();
		selected->SetBackgroundColor(Color::Red);
		tabTheme->SetStyleBox("tab_selected", "TabContainer", selected);
		tabTheme->SetColor("font_selected_color", "TabContainer", Color::Green);
		tabTheme->SetConstant("tab_height", "TabContainer", 38);
		tabTheme->SetConstant("tab_separation", "TabContainer", 7);
		tabTheme->SetConstant("page_separation", "TabContainer", 5);
		tabTree.FlushContainerUpdate();
		check(tabs->GetTabBarRect().GetHeight() == 38.f && first->GetLocalPosition().y == 43.f
			&& tabs->GetTabButton(1)->GetLocalPosition().x == tabs->GetTabButton(0)->GetSize().x + 7.f,
			"Tab theme metrics update page placement and tab spacing");
		check(tabs->GetTabButton(0)->GetThemeStyleBox("pressed") == selected
			&& tabs->GetTabButton(0)->GetThemeColor("font_pressed_color") == Color::Green,
			"Tab internal buttons use dedicated selected style and text color");
		tabs->MovePage(0, 1);
		tabTree.FlushContainerUpdate();
		check(tabs->GetCurrentPage() == first && tabs->GetCurrentTab() == 1
			&& tabs->GetTabInsertIndex(tabs->GetTabButton(1)->GetLocalPosition()) == 1,
			"Tab reordering retains selected page identity with themed geometry");
	}

	{
		SceneTree scrollTree;
		auto* scroll = MemNew<ScrollContainer>();
		auto scrollTheme = CreateRef<Theme>();
		scroll->SetTheme(scrollTheme);
		scrollTree.GetRoot()->AddNode(scroll);
		scroll->UpdateSize(Vector2(100.f, 80.f));
		scrollTree.FlushContainerUpdate();
		check(scroll->GetPageSize() == Vector2(100.f, 80.f), "empty ScrollContainer reserves no scrollbar space");
		auto* content = MemNew<Control>();
		content->SetCustomMinimumSize(Vector2(80.f, 300.f));
		scroll->AddNode(content);
		scrollTree.FlushContainerUpdate();
		check(scroll->GetPageSize() == Vector2(92.f, 80.f), "single-axis scroll reserves themed eight pixel track");
		content->SetCustomMinimumSize(Vector2(300.f, 300.f));
		scrollTree.FlushContainerUpdate();
		check(scroll->GetPageSize() == Vector2(92.f, 72.f), "dual-axis scrollbar geometry reserves both themed tracks");
		InputEvent wheel;
		wheel.m_type = InputEventType::MouseWheel;
		wheel.m_scroll = Vector2(0.f, -1.f);
		scroll->GuiInput(wheel);
		check(scroll->GetScroll() == 24.f, "scroll wheel uses compact themed step");
		scroll->SetScroll(0.f);
		InputEvent press;
		press.m_type = InputEventType::MouseButton;
		press.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
		press.m_pressed = true;
		press.m_position = Vector2(96.f, 5.f);
		check(scroll->GuiInput(press), "scrollbar drawn track accepts pointer drag");
		scrollTheme->SetConstant("scrollbar_width", "ScrollContainer", 16);
		InputEvent motion;
		motion.m_type = InputEventType::MouseMotion;
		motion.m_position = Vector2(96.f, 70.f);
		check(!scroll->GuiInput(motion) && scroll->GetScroll() == 0.f
			&& scroll->GetPageSize() == Vector2(84.f, 64.f), "scrollbar theme resize cancels old drag and recomputes page");
		scroll->SetScroll(10000.f);
		scroll->SetHorizontalScroll(10000.f);
		content->SetCustomMinimumSize(Vector2(20.f, 20.f));
		scrollTree.FlushContainerUpdate();
		check(scroll->GetPageSize() == Vector2(100.f, 80.f) && scroll->GetScroll() == 0.f
			&& scroll->GetHorizontalScroll() == 0.f, "shrinking content removes bars and clamps both scroll offsets");
		scrollTheme->SetConstant("scrollbar_width", "ScrollContainer", -4);
		scrollTheme->SetConstant("grabber_minimum_size", "ScrollContainer", 1000);
		content->SetCustomMinimumSize(Vector2(300.f, 300.f));
		scrollTree.FlushContainerUpdate();
		check(scroll->GetPageSize() == Vector2(100.f, 80.f), "negative scrollbar thickness is clamped safely");
	}

	{
		SceneTree splitTree;
		auto* split = MemNew<SplitContainer>();
		auto splitTheme = CreateRef<Theme>();
		split->SetTheme(splitTheme);
		splitTree.GetRoot()->AddNode(split);
		auto* first = MemNew<Control>();
		auto* second = MemNew<Control>();
		first->SetCustomMinimumSize(Vector2(70.f, 40.f));
		second->SetCustomMinimumSize(Vector2(50.f, 30.f));
		split->AddNode(first);
		split->AddNode(second);
		split->UpdateSize(Vector2(300.f, 160.f));
		splitTree.FlushContainerUpdate();
		check(split->GetInternalMinimumSize() == Vector2(126.f, 40.f)
			&& split->GetDividerRect().GetWidth() == 6.f
			&& second->GetLocalPosition().x == first->GetSize().x + 6.f,
			"Split compact gap is shared by minimum size, divider and panel layout");
		int finished = 0;
		split->SetDragFinishedCallback([&]() { ++finished; });
		InputEvent press;
		press.m_type = InputEventType::MouseButton;
		press.m_mouseButton = GLFW_MOUSE_BUTTON_LEFT;
		press.m_pressed = true;
		press.m_position = split->GetDividerRect().GetCenter();
		check(split->GuiInput(press), "themed divider accepts pointer drag");
		float const ratio = split->GetSplitRatio();
		splitTheme->SetConstant("separation", "SplitContainer", 20);
		splitTree.FlushContainerUpdate();
		InputEvent motion;
		motion.m_type = InputEventType::MouseMotion;
		motion.m_position = Vector2(10000.f, 80.f);
		check(!split->GuiInput(motion) && split->GetSplitRatio() == ratio && finished == 0
			&& split->GetDividerRect().GetWidth() == 20.f,
			"Split theme geometry change cancels drag without commit callback");
		press.m_position = split->GetDividerRect().GetCenter();
		split->GuiInput(press);
		split->GuiInput(motion);
		splitTree.FlushContainerUpdate();
		press.m_pressed = false;
		split->GuiInput(press);
		check(second->GetSize().x == 50.f && finished == 1,
			"Split outside drag clamps second panel minimum and commits once");
		split->SetVertical(true);
		splitTree.FlushContainerUpdate();
		check(split->GetDividerRect().GetHeight() == 20.f
			&& second->GetLocalPosition().y == first->GetSize().y + 20.f,
			"vertical Split shares the same themed separation geometry");
		split->UpdateSize(Vector2(1.f, 1.f));
		splitTree.FlushContainerUpdate();
		check(first->GetSize().y >= 40.f && second->GetSize().y >= 30.f,
			"undersized Split preserves both panel minimum sizes");
		second->SetVisible(false);
		splitTree.FlushContainerUpdate();
		check(split->GetDividerRect() == AABB2::Zero && first->GetSize() == split->GetSize(),
			"single-panel Split has no divider or reserved gap");
		split->SetDragFinishedCallback({});
	}

	auto theme           = CreateWarmCarbonInkTheme(font);
	auto other           = CreateRef<Theme>();
	other->SetFontSize("font_size", "Control", 20);
	other->SetFont("font", "Control", font);
	auto* left  = MemNew<Control>();
	auto* right = MemNew<Control>();
	left->SetTheme(theme);
	right->SetTheme(other);
	auto* button = MemNew<ThemeProbe>();
	button->SetText("Theme cache");
	left->AddNode(button);
	check(
		button->GetFont() == font && button->GetFontSize() == 12,
		"theme resolves inherited Button cache after detached attachment");
	check(
		ThemeDatabase::GetThemeItems("ThemeProbe").size() == 1
			&& ThemeDatabase::FindThemeItem("ThemeProbe", Theme::DataType::Icon, "icon") != nullptr,
		"theme metadata lists own and inherited cache items");
	check(
		ThemeDatabase::FindThemeItem("ThemeProbe", Theme::DataType::Font, "font") != nullptr,
		"theme metadata finds inherited item by category and name");
	theme->SetFont("font", "Label", font);
	button->SetFont({});
	check(
		!button->GetFont().IsValid() && button->GetChildren().empty(),
		"Button owns its text and preserves an explicitly empty font override");
	button->RemoveThemeFontOverride("font");
	auto      icon           = CreateRef<TextureResource>();
	int const iconReferences = icon->GetRefCount();
	theme->SetIcon("sample", "Button", icon);
	check(
		icon->GetRefCount() == iconReferences + 1 && button->GetThemeIcon("sample") == icon,
		"Theme retains icon resources through a strong typed reference");
	button->AddThemeIconOverride("sample", icon);
	theme->RemoveIcon("sample", "Button");
	check(
		button->HasThemeIconOverride("sample") && button->GetThemeIcon("sample") == icon,
		"icon override remains valid after removing inherited icon");
	button->RemoveThemeIconOverride("sample");
	check(
		!button->GetThemeIcon("sample").IsValid() && icon->GetRefCount() == iconReferences,
		"icon removal releases references and restores empty fallback");
	theme->SetFontSize("font_size", "Control", 0);
	check(button->GetFontSize() == 12, "invalid font size preserves existing theme value");
	theme->SetConstant("spacing", "ThemeProbe", 9);
	check(button->GetSpacing() == 9, "newly added missing theme item refreshes private derived cache");
	button->AddThemeConstantOverride("spacing", 4);
	check(button->GetSpacing() == 4, "constant override refreshes its bound derived cache");
	button->RemoveThemeConstantOverride("spacing");
	check(button->GetSpacing() == 9, "removing constant override restores inherited cache value");
	int count = button->GetRefreshCount();
	theme->SetConstant("spacing", "ThemeProbe", 9);
	check(button->GetRefreshCount() == count, "same theme value does not notify subscribers");
	theme->SetFontSize("font_size", "Control", 14);
	check(
		button->GetFontSize() == 14,
		"in-place font size update refreshes Button text metrics");
	button->SetFontSize(18);
	theme->SetFontSize("font_size", "Control", 13);
	check(button->GetFontSize() == 18, "explicit Button font size remains above Theme values");
	button->RemoveThemeFontSizeOverride("font_size");
	check(button->GetFontSize() == 13, "removing Button override restores inherited font size");
	Color const amber(211, 173, 114);
	theme->SetColor("font_color", "Button", amber);
	check(
		button->GetThemeColor("font_color") == amber,
		"normal Button text uses resolved theme color");
	button->RefreshTheme();
	check(
		button->GetThemeColor("font_color") == amber,
		"internal Label refresh retains Button text override");
	button->AddThemeColorOverride("font_color", Color::Red);
	check(button->GetThemeColor("font_color") == Color::Red, "local color override precedes inherited Theme");
	button->RemoveThemeColorOverride("font_color");
	check(button->GetThemeColor("font_color") == amber, "removing color override restores theme value");
	check(
		theme->SetTypeVariation("InkButton", "Button") && theme->SetTypeVariation("InkPrimary", "InkButton"),
		"theme accepts chained type variations");
	check(!theme->SetTypeVariation("InkButton", "InkPrimary"), "theme rejects variation cycles");
	theme->SetFontSize("font_size", "InkPrimary", 15);
	button->SetThemeTypeVariation("InkPrimary");
	check(button->GetFontSize() == 15, "theme variation precedes native class values");
	theme->RemoveFontSize("font_size", "InkPrimary");
	check(button->GetFontSize() == 13, "removing variation item falls back to native inheritance");
	button->SetThemeTypeVariation("");
	right->AddNode(button);
	check(button->GetFontSize() == 20, "reparent outside SceneTree rebuilds theme subscriptions");
	count = button->GetRefreshCount();
	theme->SetConstant("spacing", "ThemeProbe", 10);
	check(button->GetRefreshCount() == count, "reparent unsubscribes old Theme");
	left->AddNode(button);
	check(
		button->GetSpacing() == 10 && button->GetFontSize() == 13,
		"reparent restores new ancestry and missing-item dependencies");
	left->SetTheme(other);
	check(button->GetFontSize() == 20, "replacing parent Theme refreshes descendants");
	left->SetTheme(theme);
	auto* plain = MemNew<Node>();
	auto* label = MemNew<Label>();
	left->AddNode(plain);
	plain->AddNode(label);
	check(label->GetFontSize() == defaultFontSize, "ordinary Node stops theme inheritance");
	ThemeDatabase::SetDefaultTheme(other);
	check(label->GetFontSize() == 20, "default Theme replacement rebuilds registered controls");
	other->SetFontSize("font_size", "Control", 21);
	check(label->GetFontSize() == 21, "default Theme in-place changes notify controls");
	ThemeDatabase::SetDefaultTheme(originalDefault);
	check(label->GetFontSize() == defaultFontSize, "restoring default Theme clears old dependencies");
	label->SetFontSize(17);
	label->SetFont(font);
	check(label->GetFontSize() == 17 && label->GetFont() == font, "legacy Label setters establish explicit overrides");
	label->RemoveThemeFontSizeOverride("font_size");
	label->RemoveThemeFontOverride("font");
	check(
		label->GetFontSize() == defaultFontSize && label->GetFont() == originalDefault->GetDefaultFont(),
		"Label override removal restores default fallback");
	auto shared = CreateRef<ThemeStyleBox>();
	shared->SetContentMargin(ThemeStyleBox::Side::Left, 8.f);
	theme->SetStyleBox("normal", "Button", shared);
	theme->SetStyleBox("hover", "Button", shared);
	auto secondTheme = CreateRef<Theme>();
	secondTheme->SetStyleBox("normal", "Button", shared);
	right->SetTheme(secondTheme);
	auto* secondButton = MemNew<ThemeProbe>();
	right->AddNode(secondButton);
	count                 = button->GetRefreshCount();
	int const secondCount = secondButton->GetRefreshCount();
	shared->SetContentMargin(ThemeStyleBox::Side::Left, 30.f);
	check(
		button->GetRefreshCount() > count && secondButton->GetRefreshCount() > secondCount,
		"shared style box notifies all referencing Themes");
	check(button->GetInternalMinimumSize().x >= 38.f, "style content margin change updates minimum size");
	theme->RemoveStyleBox("normal", "Button");
	count = button->GetRefreshCount();
	shared->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	check(button->GetRefreshCount() > count, "shared box remains subscribed after removing one of two keys");
	theme->RemoveStyleBox("hover", "Button");
	count = button->GetRefreshCount();
	shared->SetBorderWidth(ThemeStyleBox::Side::Top, 3.f);
	check(button->GetRefreshCount() == count, "removing final style key unsubscribes that Theme");
	button->AddThemeStyleBoxOverride("normal", shared);
	button->AddThemeStyleBoxOverride("hover", shared);
	button->RemoveThemeStyleBoxOverride("normal");
	count = button->GetRefreshCount();
	shared->SetContentMargin(ThemeStyleBox::Side::Left, 40.f);
	check(button->GetRefreshCount() > count, "direct style overrides count multiple uses on one Control");
	button->RemoveThemeStyleBoxOverride("hover");
	count = button->GetRefreshCount();
	shared->SetContentMargin(ThemeStyleBox::Side::Left, 41.f);
	check(button->GetRefreshCount() == count, "removing final direct override unregisters Control");
	count = secondButton->GetRefreshCount();
	shared->SetContentMargin(ThemeStyleBox::Side::Left, 41.f);
	check(secondButton->GetRefreshCount() == count, "unchanged style value does not notify");
	auto copied = CreateRef<Theme>();
	copied->SetFontSize("font_size", "Control", 19);
	copied->SetStyleBox("normal", "Button", shared);
	auto* sourceUser = MemNew<Label>();
	sourceUser->SetTheme(copied);
	check(
		theme->CopyFrom(std::move(*copied)) && button->GetFontSize() == 19 && !copied->HasStyleBox("normal", "Button"),
		"Theme CopyFrom transfers values and clears source items");
	check(sourceUser->GetFontSize() == defaultFontSize, "Theme CopyFrom also refreshes users of the emptied source resource");
	MemDelete(sourceUser);
	count = button->GetRefreshCount();
	shared->SetBorderColor(amber);
	check(button->GetRefreshCount() > count, "Theme CopyFrom rebuilds shared style subscriptions");
	auto boxSource = CreateRef<ThemeStyleBox>();
	boxSource->SetContentMargin(ThemeStyleBox::Side::Left, 51.f);
	count = button->GetRefreshCount();
	check(
		shared->CopyFrom(std::move(*boxSource)) && button->GetRefreshCount() > count
			&& boxSource->GetMinimumSize() == Vector2::Zero,
		"ThemeStyleBox CopyFrom updates users and empties source");
	check(
		shared->GetContentRect(AABB2(0.f, 0.f, 10.f, 10.f)).GetWidth() == 0.f,
		"style content rect clamps when margins exceed available size");
	ObjectID const dead = button->GetObjectID();
	MemDelete(left);
	MemDelete(right);
	shared->SetBorderColor(Color::Red);
	theme->SetFontSize("font_size", "Control", 22);
	check(
		ObjectDatabase::GetInstance(dead) == nullptr,
		"theme and style changes after user destruction safely ignore released controls");
	int const references = shared->GetRefCount();
	{
		auto temporary = CreateRef<Theme>();
		temporary->SetStyleBox("normal", "Button", shared);
	}
	check(
		shared->GetRefCount() == references,
		"Theme destruction releases style references without subscription cycles");

	// 1) Render a small real-window specimen using a locally mounted Warm Carbon Ink Theme.
	{
		SceneTree tree;
		auto*     viewport = static_cast<Viewport*>(tree.GetRoot());
		viewport->UpdateResolution(IntVec2(720, 240));
		auto* background = MemNew<ColorRect>();
		background->SetColor(Color(36, 35, 33));
		background->UpdateSize(Vector2(720.f, 240.f));
		viewport->AddNode(background);
		auto* root = MemNew<Control>();
		root->UpdateSize(Vector2(720.f, 240.f));
		auto ink = CreateWarmCarbonInkTheme(font);
		root->SetTheme(ink);
		viewport->AddNode(root);
		auto* alternateParent = MemNew<Control>();
		alternateParent->SetTheme(other);
		viewport->AddNode(alternateParent);
		auto* movingLabel = MemNew<Label>();
		root->AddNode(movingLabel);
		alternateParent->AddNode(movingLabel);
		check(movingLabel->GetFontSize() == 21, "same-tree reparent refreshes the new theme ancestry");
		root->AddNode(movingLabel);
		check(movingLabel->GetFontSize() == 12, "same-tree reparent restores local Ink theme");
		auto* title = MemNew<Label>();
		title->SetText("Warm Carbon Ink - Theme cache / automatic refresh");
		title->SetPosition(Vector2(20.f, 16.f));
		title->UpdateSize(Vector2(680.f, 24.f));
		root->AddNode(title);
		Button*     buttons[6];
		char const* names[] = { "Normal", "Hover", "Pressed", "Hover pressed", "Disabled", "Focus" };
		for (int i = 0; i < 6; ++i)
		{
			buttons[i] = MemNew<Button>();
			buttons[i]->SetText(names[i]);
			buttons[i]->SetPosition(Vector2(20.f + i * 114.f, 60.f));
			buttons[i]->UpdateSize(Vector2(108.f, 24.f));
			root->AddNode(buttons[i]);
			buttons[i]->UpdateSize(Vector2(108.f, 24.f));
		}

		buttons[1]->Notification(Control::Notification_MouseEnter);
		buttons[2]->SetToggleMode(true);
		buttons[2]->SetPressedNoSignal(true);
		buttons[3]->SetToggleMode(true);
		buttons[3]->SetPressedNoSignal(true);
		buttons[3]->Notification(Control::Notification_MouseEnter);
		buttons[4]->SetDisabled(true);
		buttons[5]->GrabFocus();
		auto frame = [&]()
		{
			engine.BeginFrame();
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
		};
		frame();
		frame();
		std::filesystem::path const captures = projectPath.parent_path() / "Captures";
		std::filesystem::create_directories(captures);
		auto* output = engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID());
		SaveRenderedFrame(output, captures / "theme-ink-states.ppm");
		check(ReadPixel(output, 23, 63) == Color(36, 35, 33), "Ink normal state has transparent background");
		check(ReadPixel(output, 137, 63) == Color(44, 42, 39), "Ink hover state uses theme background");
		check(ReadPixel(output, 251, 63) == Color(29, 28, 26), "Ink pressed state uses theme background");
		check(ReadPixel(output, 365, 63) == Color(29, 28, 26), "Ink hover-pressed state uses theme background");
		check(ReadPixel(output, 479, 63) == Color(36, 35, 33), "Ink disabled state has transparent background");
		check(ReadPixel(output, 590, 60) == amber, "Ink focus draws an independent amber outline");
		check(
			buttons[0]->GetSize().y == 24.f && buttons[0]->GetFontSize() == 12,
			"Ink specimen keeps 24px controls and 12px text");
		ink->GetStyleBox("hover", "Button")->SetBackgroundColor(Color(60, 51, 38));
		ink->SetColor("font_hover_color", "Button", amber);
		ink->SetFontSize("font_size", "Control", 14);
		frame();
		SaveRenderedFrame(output, captures / "theme-ink-updated.ppm");
		check(
			ReadPixel(output, 137, 63) == Color(60, 51, 38),
			"GPU draw updates automatically after shared style mutation");
		check(
			buttons[1]->GetThemeColor("font_hover_color") == amber
				&& buttons[1]->GetFontSize() == 14,
			"live theme edits update state text and font cache");
		ink->GetStyleBox("normal", "Button")->SetContentMargin(ThemeStyleBox::Side::Top, 20.f);
		check(
			buttons[0]->GetSize().y >= buttons[0]->GetInternalMinimumSize().y && buttons[0]->GetSize().y > 24.f,
			"live margin changes update layout and hit size together");
		frame();
		SaveRenderedFrame(output, captures / "theme-ink-layout-updated.ppm");
		if (std::getenv("MING_BUTTON_THEME_PREVIEW") != nullptr)
		{
			ink = CreateWarmCarbonInkTheme(font);
			root->SetTheme(ink);
			for (auto* sample : buttons)
			{
				sample->UpdateSize(Vector2(108.f, 24.f));
			}

			bool  done    = false;
			bool  changed = false;
			auto* update  = MemNew<Button>();
			update->SetText("Update theme");
			update->SetPosition(Vector2(20.f, 120.f));
			root->AddNode(update);
			update->SetPressedCallback(
				[&]()
				{
					changed = !changed;
					ink->SetFontSize("font_size", "Control", changed ? 14 : 12);
					ink->SetColor("font_color", "Button", changed ? amber : Color(222, 217, 208));
					ink->GetStyleBox("normal", "Button")
						->SetContentMargin(ThemeStyleBox::Side::Top, changed ? 12.f : 4.f);
					title->SetText(
						changed ? "Updated: 14px text / amber / larger top margin"
								: "Warm Carbon Ink - Theme cache / automatic refresh");
				});
			auto* close = MemNew<Button>();
			close->SetText("Close preview");
			close->SetPosition(Vector2(200.f, 120.f));
			root->AddNode(close);
			close->SetPressedCallback(
				[&]()
				{
					done = true;
				});
			glfwRestoreWindow(engine.m_windowSystem->GetGLFWWindow());
			glfwSetWindowSize(engine.m_windowSystem->GetGLFWWindow(), 720, 240);
			while (!done && !glfwWindowShouldClose(engine.m_windowSystem->GetGLFWWindow()))
			{
				frame();
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		}

		ObjectID const rootID = root->GetObjectID();
		root->QueueFree();
		ink->SetFontSize("font_size", "Control", 16);
		tree.FlushPendingNode();
		ink->SetFontSize("font_size", "Control", 17);
		check(ObjectDatabase::GetInstance(rootID) == nullptr, "queued specimen deletion releases theme users");
	}

	// 2) Exercise all five controls together with a locally mutable theme and real rendering.
	{
		SceneTree tree;
		auto* viewport = static_cast<Viewport*>(tree.GetRoot());
		viewport->UpdateResolution(IntVec2(1040, 680));
		auto ink = CreateWarmCarbonInkTheme(font);
		auto copyStyle = [&](char const* name, char const* type)
		{
			auto const source = originalDefault->GetStyleBox(name, type);
			auto copy = CreateRef<ThemeStyleBox>();
			copy->SetBackgroundColor(source->GetBackgroundColor());
			copy->SetBorderColor(source->GetBorderColor());
			for (int side = 0; side < 4; ++side)
			{
				auto const edge = static_cast<ThemeStyleBox::Side>(side);
				copy->SetBorderWidth(edge, source->GetBorderWidth(edge));
				copy->SetContentMargin(edge, source->GetContentMargin(edge));
			}

			ink->SetStyleBox(name, type, copy);
			return copy;
		};
		auto inputStyle = copyStyle("normal", "LineEdit");
		auto selectedStyle = copyStyle("selected", "Tree");
		auto tabStyle = copyStyle("tab_selected", "TabContainer");
		auto grabberStyle = copyStyle("grabber", "ScrollContainer");
		auto dividerStyle = copyStyle("normal", "SplitContainer");
		auto* background = MemNew<ColorRect>();
		background->SetColor(Color(25, 24, 23));
		viewport->AddNode(background);
		auto* root = MemNew<Control>();
		root->SetTheme(ink);
		viewport->AddNode(root);
		auto* title = MemNew<Label>();
		title->SetText("Warm Carbon Ink | five controls / live theme reflow");
		title->SetPosition(Vector2(20.f, 16.f));
		root->AddNode(title);
		auto* help = MemNew<Label>();
		help->SetText("F2 renames a tree row. Drag tracks and dividers. Switch tabs and resize the window.");
		help->SetPosition(Vector2(20.f, 48.f));
		root->AddNode(help);
		auto* update = MemNew<Button>();
		update->SetText("Update theme");
		update->SetPosition(Vector2(20.f, 86.f));
		update->UpdateSize(Vector2(164.f, 30.f));
		root->AddNode(update);
		auto* close = MemNew<Button>();
		close->SetText("Close preview");
		close->SetPosition(Vector2(196.f, 86.f));
		close->UpdateSize(Vector2(164.f, 30.f));
		root->AddNode(close);
		auto* normal = MemNew<LineEdit>();
		normal->SetText("Edit this draft, then click Update theme");
		normal->SetPosition(Vector2(20.f, 142.f));
		normal->UpdateSize(Vector2(380.f, 30.f));
		root->AddNode(normal);
		auto* readOnly = MemNew<LineEdit>();
		readOnly->SetText("Read only");
		readOnly->SetEditable(false);
		readOnly->SetPosition(Vector2(420.f, 142.f));
		readOnly->UpdateSize(Vector2(220.f, 30.f));
		root->AddNode(readOnly);
		auto* disabled = MemNew<LineEdit>();
		disabled->SetPlaceholder("Disabled field");
		disabled->SetDisabled(true);
		disabled->SetPosition(Vector2(660.f, 142.f));
		disabled->UpdateSize(Vector2(220.f, 30.f));
		root->AddNode(disabled);
		auto* split = MemNew<SplitContainer>();
		split->SetPosition(Vector2(20.f, 202.f));
		split->SetSplitRatio(0.42f);
		root->AddNode(split);
		auto* scroll = MemNew<ScrollContainer>();
		split->AddNode(scroll);
		auto* rows = MemNew<Tree>();
		scroll->AddNode(rows);
		std::vector<Tree::Item> items;
		items.push_back({ 1, 0, "Scene root", true, true });
		for (uint64_t id = 2; id <= 28; ++id)
		{
			items.push_back({ id, 1, String("Editable item ") + String(std::to_string(id).c_str())
				+ " - horizontally scrollable content", true, true });
		}

		rows->SetItems(items);
		rows->SetRenamedCallback([rows, items = std::move(items)](uint64_t id, String const& text) mutable
		{
			for (auto& item : items)
			{
				if (item.m_ID == id)
				{
					item.m_text = text;
				}
			}

			rows->SetItems(items);
		});
		auto* tabs = MemNew<TabContainer>();
		split->AddNode(tabs);
		auto* fields = MemNew<VBoxContainer>();
		fields->SetSeparation(8.f);
		tabs->AddPage("Fields", fields);
		auto* note = MemNew<Label>();
		note->SetText("Theme changes preserve text and selection");
		fields->AddNode(note);
		auto* draft = MemNew<LineEdit>();
		draft->SetText("An editable draft inside a themed tab");
		fields->AddNode(draft);
		auto* placeholder = MemNew<LineEdit>();
		placeholder->SetPlaceholder("Placeholder text");
		fields->AddNode(placeholder);
		auto* vertical = MemNew<SplitContainer>();
		vertical->SetVertical(true);
		vertical->SetCustomMinimumSize(Vector2(100.f, 130.f));
		vertical->SetVerticalSizeFlags(Control::Size_ExpandFill);
		fields->AddNode(vertical);
		auto* upper = MemNew<ColorRect>();
		upper->SetColor(Color(36, 35, 33));
		upper->SetCustomMinimumSize(Vector2(60.f, 40.f));
		vertical->AddNode(upper);
		auto* lower = MemNew<ColorRect>();
		lower->SetColor(Color(29, 28, 26));
		lower->SetCustomMinimumSize(Vector2(60.f, 40.f));
		vertical->AddNode(lower);
		auto* pageScroll = MemNew<ScrollContainer>();
		tabs->AddPage("Scroll", pageScroll);
		auto* content = MemNew<VBoxContainer>();
		content->SetCustomMinimumSize(Vector2(800.f, 700.f));
		pageScroll->AddNode(content);
		for (int i = 0; i < 25; ++i)
		{
			auto* label = MemNew<Label>();
			label->SetText(String("Scrollable content row ") + String(std::to_string(i + 1).c_str()));
			content->AddNode(label);
		}

		tabs->AddPage("Empty", MemNew<Control>());
		auto frame = [&]()
		{
			engine.BeginFrame();
			Vector2 const size(viewport->GetOutputResolution());
			root->UpdateSize(size);
			background->UpdateSize(size);
			split->UpdateSize(Vector2(std::max(300.f, size.x - 40.f), std::max(180.f, size.y - 222.f)));
			tree.UpdateScene(0.016f);
			engine.m_renderServer->Render();
			engine.EndFrame();
		};
		bool expanded = false;
		auto apply = [&](bool large)
		{
			ink->SetFontSize("font_size", "Control", large ? 16 : 12);
			ink->SetConstant("row_height", "Tree", large ? 34 : 24);
			ink->SetConstant("indent", "Tree", large ? 22 : 14);
			ink->SetConstant("tab_height", "TabContainer", large ? 36 : 26);
			ink->SetConstant("scrollbar_width", "ScrollContainer", large ? 14 : 8);
			ink->SetConstant("separation", "SplitContainer", large ? 12 : 6);
			inputStyle->SetContentMargin(ThemeStyleBox::Side::Left, large ? 12.f : 6.f);
			inputStyle->SetContentMargin(ThemeStyleBox::Side::Right, large ? 12.f : 6.f);
			inputStyle->SetContentMargin(ThemeStyleBox::Side::Top, large ? 6.f : 3.f);
			inputStyle->SetContentMargin(ThemeStyleBox::Side::Bottom, large ? 6.f : 3.f);
			selectedStyle->SetBackgroundColor(large ? Color(81, 66, 45) : Color(60, 51, 38));
			tabStyle->SetBorderWidth(ThemeStyleBox::Side::Top, large ? 3.f : 2.f);
			grabberStyle->SetBackgroundColor(large ? Color(211, 173, 114) : Color(81, 74, 64));
			dividerStyle->SetBackgroundColor(large ? Color(60, 51, 38) : Color(25, 24, 23));
		};
		std::filesystem::path const captures = projectPath.parent_path() / "Captures";
		frame();
		frame();
		rows->Select(2);
		frame();
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID()), captures / "theme-controls-compact.ppm");
		check(rows->GetRowHeight() == 24.f && tabs->GetTabBarRect().GetHeight() == 26.f
			&& split->GetDividerRect().GetWidth() == 6.f, "combined compact specimen uses shared default metrics");
		rows->EditSelected();
		auto* rename = static_cast<LineEdit*>(rows->GetChildren()[0]);
		rename->Select(1, 4);
		apply(true);
		frame();
		SaveRenderedFrame(engine.m_renderServer->ViewportGetTexture(viewport->GetViewportRID()), captures / "theme-controls-expanded.ppm");
		check(rows->GetRowHeight() >= 34.f && tabs->GetTabBarRect().GetHeight() >= 36.f
			&& split->GetDividerRect().GetWidth() == 12.f && rename->GetSelectionFromColumn() == 1
			&& rename->GetCursorColumn() == 4 && rename->IsEditing(), "combined live reflow preserves rename while all five controls resize");
		rename->EndEditing(LineEdit::EditEndReason::Cancelled);
		apply(false);
		bool done = false;
		update->SetPressedCallback([&]()
		{
			expanded = !expanded;
			apply(expanded);
		});
		close->SetPressedCallback([&]() { done = true; });
		if (std::getenv("MING_THEME_PREVIEW") != nullptr)
		{
			glfwRestoreWindow(engine.m_windowSystem->GetGLFWWindow());
			glfwSetWindowTitle(engine.m_windowSystem->GetGLFWWindow(), "Ming Theme Controls Verification");
			glfwSetWindowSize(engine.m_windowSystem->GetGLFWWindow(), 1040, 680);
			while (!done && !glfwWindowShouldClose(engine.m_windowSystem->GetGLFWWindow()))
			{
				frame();
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		}

		update->SetPressedCallback({});
		close->SetPressedCallback({});
		root->QueueFree();
		tree.FlushPendingNode();
	}

}
