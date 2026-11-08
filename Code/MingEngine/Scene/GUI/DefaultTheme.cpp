#include "MingEngine/Scene/GUI/DefaultTheme.hpp"

Ref<Theme> CreateDefaultTheme()
{
	constexpr int   controlHeight        = 24;
	constexpr int   iconSize             = 16;
	constexpr int   compactSpacing       = 4;
	constexpr int   iconSeparation       = 6;
	constexpr float contentPadding       = 8.f;
	constexpr float fieldPadding         = 6.f;
	constexpr float fieldVerticalPadding = 3.f;

	auto        theme = CreateRef<Theme>();
	Color const background(30, 30, 30);
	Color const panel(40, 40, 40);
	Color const elevated(48, 48, 48);
	Color const field(24, 24, 24);
	Color const border(64, 64, 64);
	Color const strong(86, 86, 86);
	Color const text(224, 224, 224);
	Color const muted(165, 165, 165);
	Color const dim(125, 125, 125);
	Color const accent(130, 165, 210);
	Color const tint(48, 62, 80);
	// Create independent mutable boxes so editing one state does not change another state.
	// e.g. Changing LineEdit normal margins leaves its hover resource unchanged.
	auto box = [](Color const& fill, Color const& edge, float width, float horizontal, float vertical)
	{
		auto style = CreateRef<ThemeStyleBox>();
		style->SetBackgroundColor(fill);
		style->SetBorderColor(edge);
		for (int side = 0; side < 4; ++side)
		{
			style->SetBorderWidth(static_cast<ThemeStyleBox::Side>(side), width);
			style->SetContentMargin(static_cast<ThemeStyleBox::Side>(side), side % 2 == 0 ? horizontal : vertical);
		}

		return style;
	};

	theme->SetDefaultFont(FontResource::CreateDefault());
	theme->SetDefaultFontSize(16);
	theme->SetColor("font_color", "Control", text);
	// StyleBox margins inset contents; container separation only spaces siblings.
	// e.g. A PanelContainer already reserves its StyleBox padding around a VBoxContainer.
	theme->SetConstant("line_spacing", "Label", 0);
	theme->SetConstant("separation", "BoxContainer", 0);
	for (char const* margin : { "margin_left", "margin_top", "margin_right", "margin_bottom" })
	{
		theme->SetConstant(margin, "MarginContainer", 0);
	}

	theme->SetConstant("minimum_height", "Button", controlHeight);
	theme->SetConstant("minimum_height", "ProgressBar", 12);
	theme->SetStyleBox("background", "ProgressBar", box(field, border, 1.f, 1.f, 1.f));
	theme->SetStyleBox("fill", "ProgressBar", box(accent, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetTypeVariation("MenuBarButton", "Button");
	theme->SetConstant("minimum_height", "MenuBar", 24);
	theme->SetConstant("popup_minimum_width", "MenuBar", 200);
	theme->SetStyleBox(
		"normal",
		"MenuBarButton",
		box(Color::Transparent, Color::Transparent, 0.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox(
		"normal",
		"Button",
		box(elevated, border, 1.f, contentPadding, static_cast<float>(compactSpacing)));
	theme->SetStyleBox("hover", "Button", box(strong, strong, 1.f, contentPadding, static_cast<float>(compactSpacing)));
	theme->SetStyleBox("pressed", "Button", box(tint, strong, 1.f, contentPadding, static_cast<float>(compactSpacing)));
	theme->SetStyleBox(
		"hover_pressed",
		"Button",
		box(tint, accent, 1.f, contentPadding, static_cast<float>(compactSpacing)));
	theme->SetStyleBox(
		"disabled",
		"Button",
		box(Color::Transparent, Color::Transparent, 0.f, contentPadding, static_cast<float>(compactSpacing)));
	theme->SetStyleBox("focus", "Button", box(Color::Transparent, accent, 1.f, 0.f, 0.f));
	theme->SetColor("font_hover_color", "Button", text);
	theme->SetColor("font_pressed_color", "Button", accent);
	theme->SetColor("font_hover_pressed_color", "Button", accent);
	theme->SetColor("font_disabled_color", "Button", dim);

	theme->SetConstant("minimum_height", "LineEdit", controlHeight);
	theme->SetConstant("caret_width", "LineEdit", 1);
	theme->SetColor("font_read_only_color", "LineEdit", muted);
	theme->SetColor("font_disabled_color", "LineEdit", dim);
	theme->SetColor("font_placeholder_color", "LineEdit", dim);
	theme->SetColor("selection_color", "LineEdit", tint);
	theme->SetColor("caret_color", "LineEdit", accent);
	theme->SetStyleBox("normal", "LineEdit", box(field, border, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("hover", "LineEdit", box(field, strong, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("read_only", "LineEdit", box(panel, border, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("disabled", "LineEdit", box(field, border, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("focus", "LineEdit", box(Color::Transparent, accent, 1.f, 0.f, 0.f));

	theme->SetConstant("row_height", "Tree", controlHeight);
	theme->SetConstant("indent", "Tree", 14);
	theme->SetConstant("icon_size", "Tree", iconSize);
	theme->SetConstant("fold_size", "Tree", 12);
	theme->SetConstant("icon_separation", "Tree", compactSpacing);
	theme->SetColor("font_selected_color", "Tree", text);
	theme->SetStyleBox("panel", "Tree", box(Color::Transparent, Color::Transparent, 0.f, 4.f, 0.f));
	theme->SetStyleBox("hover", "Tree", box(elevated, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("selected", "Tree", box(tint, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("focus", "Tree", box(Color::Transparent, accent, 1.f, 0.f, 0.f));
	auto drop = box(Color::Transparent, accent, 0.f, 0.f, 0.f);
	drop->SetBorderWidth(ThemeStyleBox::Side::Top, 1.f);
	drop->SetBorderWidth(ThemeStyleBox::Side::Bottom, 1.f);
	theme->SetStyleBox("drop_mark", "Tree", drop);

	theme->SetConstant("tab_height", "TabContainer", 26);
	theme->SetConstant("tab_minimum_width", "TabContainer", 48);
	theme->SetConstant("tab_separation", "TabContainer", 0);
	theme->SetConstant("page_separation", "TabContainer", 0);
	theme->SetConstant("drop_mark_width", "TabContainer", 2);
	theme->SetColor("drop_mark_color", "TabContainer", accent);
	theme->SetStyleBox("panel", "TabContainer", box(panel, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("tab_normal", "TabContainer", box(field, Color::Transparent, 0.f, 10.f, 4.f));
	theme->SetStyleBox("tab_hover", "TabContainer", box(elevated, Color::Transparent, 0.f, 10.f, 4.f));
	auto selected = box(panel, accent, 0.f, 10.f, 4.f);
	selected->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	theme->SetStyleBox("tab_selected", "TabContainer", selected);
	auto selectedHover = box(panel, accent, 0.f, 10.f, 4.f);
	selectedHover->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	theme->SetStyleBox("tab_selected_hover", "TabContainer", selectedHover);
	theme->SetStyleBox("tab_disabled", "TabContainer", box(field, Color::Transparent, 0.f, 10.f, 4.f));
	theme->SetStyleBox("tab_focus", "TabContainer", box(Color::Transparent, accent, 1.f, 0.f, 0.f));
	theme->SetColor("font_color", "TabContainer", muted);
	theme->SetColor("font_hover_color", "TabContainer", text);
	theme->SetColor("font_selected_color", "TabContainer", text);
	theme->SetColor("font_selected_hover_color", "TabContainer", text);
	theme->SetColor("font_disabled_color", "TabContainer", dim);

	theme->SetColor("background_color", "ScrollContainer", panel);
	theme->SetConstant("scrollbar_width", "ScrollContainer", 8);
	theme->SetConstant("grabber_minimum_size", "ScrollContainer", 20);
	theme->SetConstant("grabber_margin", "ScrollContainer", 2);
	theme->SetConstant("scroll_step", "ScrollContainer", controlHeight);
	theme->SetStyleBox("track", "ScrollContainer", box(panel, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("grabber", "ScrollContainer", box(strong, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("grabber_hover", "ScrollContainer", box(muted, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("grabber_pressed", "ScrollContainer", box(accent, Color::Transparent, 0.f, 0.f, 0.f));

	theme->SetConstant("separation", "SplitContainer", 3);
	theme->SetConstant("grabber_length", "SplitContainer", 25);
	theme->SetConstant("grabber_width", "SplitContainer", 1);
	theme->SetConstant("grabber_separation", "SplitContainer", 4);
	theme->SetColor("grabber_color", "SplitContainer", strong);
	theme->SetColor("grabber_hover_color", "SplitContainer", accent);
	theme->SetColor("grabber_pressed_color", "SplitContainer", accent);
	theme->SetStyleBox("normal", "SplitContainer", box(background, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("hover", "SplitContainer", box(tint, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("pressed", "SplitContainer", box(tint, Color::Transparent, 0.f, 0.f, 0.f));

	theme->SetConstant("icon_size", "Button", iconSize);
	theme->SetConstant("icon_separation", "Button", iconSeparation);
	theme->SetStyleBox("panel", "PanelContainer", box(panel, border, 1.f, contentPadding, contentPadding));
	theme->SetConstant("check_size", "MenuBar", 15);
	theme->SetConstant("item_padding", "MenuBar", 10);
	theme->SetConstant("shortcut_separation", "MenuBar", 25);
	theme->SetConstant("separator_width", "MenuBar", 1);
	theme->SetConstant("icon_separation", "LineEdit", 8);
	theme->SetTypeVariation("SpinBoxStep", "Button");
	theme->SetFontSize("font_size", "SpinBoxStep", 12);
	theme->SetTypeVariation("SpinBoxError", "Label");
	theme->SetFontSize("font_size", "SpinBoxError", 11);
	theme->SetConstant("minimum_width", "ColorPicker", 320);
	theme->SetConstant("shape_height", "ColorPicker", 200);
	theme->SetConstant("bar_width", "ColorPicker", 20);
	theme->SetConstant("row_height", "ColorPicker", 32);
	theme->SetConstant("spacing", "ColorPicker", 8);
	theme->SetConstant("padding", "ColorPicker", 8);
	theme->SetConstant("checker_size", "ColorPicker", 6);
	theme->SetColor("checker_light", "ColorPicker", Color(160, 160, 160));
	theme->SetColor("checker_dark", "ColorPicker", Color(100, 100, 100));
	theme->SetColor("cursor_color", "ColorPicker", Color::White);
	theme->SetStyleBox("panel", "ColorPicker", box(panel, border, 1.f, 0.f, 0.f));
	return theme;
}
