#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/BuiltinIcons.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"

Ref<Theme> EditorThemeManager::CreateTheme()
{
	constexpr int   controlHeight        = 30;
	constexpr int   iconSize             = 20;
	constexpr int   compactSpacing       = 5;
	constexpr int   iconSeparation       = 8;
	constexpr float contentPadding       = 10.f;
	constexpr float fieldPadding         = 7.5f;
	constexpr float fieldVerticalPadding = 3.75f;

	auto        theme = CreateRef<Theme>();
	Color const background(25, 24, 23);
	Color const panel(36, 35, 33);
	Color const elevated(44, 42, 39);
	Color const field(29, 28, 26);
	Color const border(57, 54, 49);
	Color const strong(81, 74, 64);
	Color const text(222, 217, 208);
	Color const muted(164, 158, 147);
	Color const dim(127, 121, 110);
	Color const accent(211, 173, 114);
	Color const tint(60, 51, 38);
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
			style->SetContentMargin(
				static_cast<ThemeStyleBox::Side>(side),
				side % 2 == 0 ? horizontal : vertical);
		}

		return style;
	};

	theme->SetDefaultFont(FontResource::CreateDefault(true));
	theme->SetDefaultFontSize(15);
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
	theme->SetStyleBox("normal", "LineEdit", box(field, Color::Transparent, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("hover", "LineEdit", box(field, strong, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("read_only", "LineEdit", box(panel, border, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("disabled", "LineEdit", box(field, border, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("focus", "LineEdit", box(Color::Transparent, accent, 1.f, 0.f, 0.f));

	theme->SetConstant("row_height", "Tree", controlHeight);
	theme->SetConstant("indent", "Tree", 18);
	theme->SetConstant("icon_size", "Tree", iconSize);
	theme->SetConstant("fold_size", "Tree", 15);
	theme->SetConstant("icon_separation", "Tree", compactSpacing);
	theme->SetColor("font_selected_color", "Tree", text);
	theme->SetStyleBox("panel", "Tree", box(Color::Transparent, Color::Transparent, 0.f, 5.f, 0.f));
	theme->SetStyleBox("hover", "Tree", box(elevated, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("selected", "Tree", box(tint, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("focus", "Tree", box(Color::Transparent, Color::Transparent, 0.f, 0.f, 0.f));
	auto drop = box(Color::Transparent, accent, 0.f, 0.f, 0.f);
	drop->SetBorderWidth(ThemeStyleBox::Side::Top, 1.f);
	drop->SetBorderWidth(ThemeStyleBox::Side::Bottom, 1.f);
	theme->SetStyleBox("drop_mark", "Tree", drop);

	theme->SetConstant("tab_height", "TabContainer", 33);
	theme->SetConstant("tab_minimum_width", "TabContainer", 60);
	theme->SetConstant("tab_separation", "TabContainer", 0);
	theme->SetConstant("page_separation", "TabContainer", 0);
	theme->SetConstant("drop_mark_width", "TabContainer", 3);
	theme->SetColor("drop_mark_color", "TabContainer", accent);
	theme->SetStyleBox("panel", "TabContainer", box(panel, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("tab_normal", "TabContainer", box(field, Color::Transparent, 0.f, 12.5f, 5.f));
	theme->SetStyleBox("tab_hover", "TabContainer", box(elevated, Color::Transparent, 0.f, 12.5f, 5.f));
	auto selected = box(panel, accent, 0.f, 12.5f, 5.f);
	selected->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	theme->SetStyleBox("tab_selected", "TabContainer", selected);
	auto selectedHover = box(panel, accent, 0.f, 12.5f, 5.f);
	selectedHover->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	theme->SetStyleBox("tab_selected_hover", "TabContainer", selectedHover);
	theme->SetStyleBox("tab_disabled", "TabContainer", box(field, Color::Transparent, 0.f, 12.5f, 5.f));
	theme->SetStyleBox("tab_focus", "TabContainer", box(Color::Transparent, accent, 1.f, 0.f, 0.f));
	theme->SetColor("font_color", "TabContainer", muted);
	theme->SetColor("font_hover_color", "TabContainer", text);
	theme->SetColor("font_selected_color", "TabContainer", text);
	theme->SetColor("font_selected_hover_color", "TabContainer", text);
	theme->SetColor("font_disabled_color", "TabContainer", dim);

	theme->SetColor("background_color", "ScrollContainer", panel);
	theme->SetConstant("scrollbar_width", "ScrollContainer", 10);
	theme->SetConstant("grabber_minimum_size", "ScrollContainer", 25);
	theme->SetConstant("grabber_margin", "ScrollContainer", 3);
	theme->SetConstant("scroll_step", "ScrollContainer", controlHeight);
	theme->SetStyleBox("track", "ScrollContainer", box(panel, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("grabber", "ScrollContainer", box(strong, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("grabber_hover", "ScrollContainer", box(muted, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("grabber_pressed", "ScrollContainer", box(accent, Color::Transparent, 0.f, 0.f, 0.f));

	theme->SetConstant("separation", "SplitContainer", 8);
	theme->SetConstant("grabber_length", "SplitContainer", 18);
	theme->SetConstant("grabber_width", "SplitContainer", 1);
	theme->SetConstant("grabber_separation", "SplitContainer", 5);
	theme->SetColor("grabber_color", "SplitContainer", strong);
	theme->SetColor("grabber_hover_color", "SplitContainer", accent);
	theme->SetColor("grabber_pressed_color", "SplitContainer", accent);
	theme->SetStyleBox("normal", "SplitContainer", box(background, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("hover", "SplitContainer", box(tint, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetStyleBox("pressed", "SplitContainer", box(tint, Color::Transparent, 0.f, 0.f, 0.f));

	theme->SetTypeVariation("EditorToolbarButton", "Button");
	theme->SetTypeVariation("MenuBarButton", "EditorToolbarButton");
	theme->SetConstant("minimum_height", "MenuBar", 35);
	theme->SetConstant("popup_minimum_width", "MenuBar", 250);
	theme->SetConstant("top_bar_height", "Editor", 60);
	theme->SetConstant("status_bar_height", "Editor", 28);
	theme->SetTypeVariation("EditorPrimaryButton", "Button");
	theme->SetTypeVariation("EditorWorkspaceButton", "Button");
	theme->SetTypeVariation("EditorDockTabs", "TabContainer");
	theme->SetTypeVariation("EditorInspectorTitle", "Label");
	theme->SetTypeVariation("EditorPropertyTitle", "Label");
	theme->SetTypeVariation("EditorErrorLabel", "Label");
	theme->SetFontSize("font_size", "EditorInspectorTitle", 15);
	theme->SetFontSize("font_size", "EditorPropertyTitle", 14);
	theme->SetFontSize("font_size", "EditorErrorLabel", 13);
	theme->SetColor("font_color", "EditorErrorLabel", Color(225, 120, 105));
	theme->SetStyleBox(
		"normal",
		"EditorToolbarButton",
		box(Color::Transparent, Color::Transparent, 0.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox(
		"hover",
		"EditorToolbarButton",
		box(elevated, Color::Transparent, 0.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("pressed", "EditorToolbarButton", box(field, strong, 1.f, fieldPadding, fieldVerticalPadding));
	theme->SetStyleBox("normal", "EditorWorkspaceButton", box(tint, strong, 1.f, 12.5f, 3.75f));
	theme->SetStyleBox("hover", "EditorWorkspaceButton", box(tint, strong, 1.f, 12.5f, 3.75f));
	theme->SetColor("font_color", "EditorWorkspaceButton", accent);
	theme->SetStyleBox("normal", "EditorPrimaryButton", box(accent, accent, 1.f, 12.5f, 5.f));
	theme->SetStyleBox("hover", "EditorPrimaryButton", box(Color(226, 186, 114), Color(226, 186, 114), 1.f, 12.5f, 5.f));
	theme->SetStyleBox("pressed", "EditorPrimaryButton", box(accent, background, 1.f, 12.5f, 5.f));
	theme->SetStyleBox("hover_pressed", "EditorPrimaryButton", box(accent, background, 1.f, 12.5f, 5.f));
	theme->SetColor("font_color", "EditorPrimaryButton", background);
	theme->SetColor("font_hover_color", "EditorPrimaryButton", background);
	theme->SetColor("font_pressed_color", "EditorPrimaryButton", background);
	theme->SetColor("font_hover_pressed_color", "EditorPrimaryButton", background);
	theme->SetColor("background", "Editor", background);
	theme->SetColor("panel", "Editor", panel);
	theme->SetColor("elevated", "Editor", elevated);
	theme->SetColor("accent", "Editor", accent);
	theme->SetColor("scene_background", "Editor", Color(32, 32, 32));
	theme->SetConstant("icon_size", "Button", iconSize);
	theme->SetConstant("icon_separation", "Button", iconSeparation);
	theme->SetStyleBox("panel", "PanelContainer", box(panel, border, 1.f, contentPadding, contentPadding));

	for (ClassInfo const* info : ClassDatabase::GetRegisteredClasses())
	{
		if (info != nullptr && info->m_apiType == ApiType::Runtime
			&& ClassDatabase::IsSubclassOf(info->m_className, "Node"))
		{
			theme->SetIcon(info->m_className, "EditorIcons", EditorIcons::GetIconTexture(info->m_className, "Node"));
		}
	}

	theme->SetConstant("guide_width", "Tree", 1);
	theme->SetIcon("arrow_collapsed", "Tree", EditorIcons::GetIconTexture("ChevronRight"));
	theme->SetIcon("arrow_expanded", "Tree", EditorIcons::GetIconTexture("ChevronDown"));
	theme->SetColor("guide_color", "Tree", strong);
	theme->SetColor("tn", "EditorIcons", muted);
	theme->SetColor("t2d", "EditorIcons", Color(147, 208, 221));
	theme->SetColor("t3d", "EditorIcons", Color(166, 139, 212));
	theme->SetColor("tui", "EditorIcons", Color(159, 190, 160));
	theme->SetColor("terr", "EditorIcons", Color(227, 152, 152));
	theme->SetColor("twarn", "EditorIcons", Color(226, 186, 114));
	theme->SetColor("tsuc", "EditorIcons", Color(145, 185, 155));
	for (auto const& icon : BuiltinIcons::Detail::kEntries)
	{
		theme->SetIcon(std::string(icon.m_name), "EditorIcons", EditorIcons::GetIconTexture(std::string(icon.m_name)));
	}

	theme->SetStyleBox("panel", "EditorPanel", box(panel, Color::Transparent, 0.f, 0.f, 0.f));
	return theme;
}
