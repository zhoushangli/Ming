#include "MingEngine/Editor/EditorThemeManager.hpp"
#include "MingEngine/Editor/UI/EditorIcons.hpp"
#include "MingEngine/Editor/BuiltinIcons.hpp"
#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Editor/UI/JetBrainsMonoFont.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

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
	theme->SetTypeVariation("EditorInspectorClassHeader", "PanelContainer");
	theme->SetTypeVariation("EditorInspectorClassLabel", "Label");
	theme->SetTypeVariation("EditorStatusLabel", "Label");
	theme->SetStyleBox("panel", "EditorInspectorClassHeader", box(elevated, Color::Transparent, 0.f, 5.f, 5.f));
	theme->SetFontSize("font_size", "EditorInspectorClassLabel", 15);
	theme->SetFontSize("font_size", "EditorStatusLabel", 13);
	theme->SetColor("font_color", "EditorStatusLabel", dim);
	theme->SetColor("font_color", "EditorPropertyTitle", muted);
	theme->SetFont("ruler_font", "ViewportPanel", theme->GetDefaultFont());
	theme->SetFontSize("ruler_font_size", "ViewportPanel", 13);
	theme->SetColor("ruler_background", "ViewportPanel", field);
	theme->SetColor("ruler_tick_color", "ViewportPanel", border);
	theme->SetColor("ruler_major_color", "ViewportPanel", strong);
	theme->SetColor("ruler_font_color", "ViewportPanel", dim);
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

	theme->SetTypeVariation("EditorViewportHeader", "PanelContainer");
	theme->SetStyleBox("panel", "EditorViewportHeader", box(field, Color::Transparent, 0.f, 0.f, 0.f));
	theme->SetTypeVariation("EditorSceneTab", "PanelContainer");
	auto sceneTab = box(panel, accent, 0.f, 7.5f, 0.f);
	sceneTab->SetBorderWidth(ThemeStyleBox::Side::Top, 2.f);
	sceneTab->SetContentMargin(ThemeStyleBox::Side::Top, 2.f);
	theme->SetStyleBox("panel", "EditorSceneTab", sceneTab);
	theme->SetTypeVariation("EditorMenuPopup", "PanelContainer");
	theme->SetStyleBox("panel", "EditorMenuPopup", box(panel, border, 1.f, 2.5f, 3.75f));
	theme->SetTypeVariation("EditorPanelAction", "EditorToolbarButton");
	theme->SetConstant("minimum_height", "EditorPanelAction", 28);
	theme->SetStyleBox("normal", "EditorPanelAction", box(Color::Transparent, Color::Transparent, 0.f, 3.75f, 2.5f));
	theme->SetStyleBox("hover", "EditorPanelAction", box(elevated, Color::Transparent, 0.f, 3.75f, 2.5f));
	theme->SetColor("font_color", "EditorPanelAction", dim);
	theme->SetConstant("minimum_height", "EditorToolbarButton", 30);
	theme->SetColor("font_color", "EditorToolbarButton", muted);
	theme->SetTypeVariation("EditorMenuItem", "Button");
	theme->SetTypeVariation("EditorOutputTab", "Button");
	theme->SetStyleBox("normal", "EditorOutputTab", theme->GetStyleBox("tab_normal", "TabContainer"));
	theme->SetStyleBox("hover", "EditorOutputTab", theme->GetStyleBox("tab_hover", "TabContainer"));
	theme->SetStyleBox("pressed", "EditorOutputTab", theme->GetStyleBox("tab_selected", "TabContainer"));
	theme->SetStyleBox("hover_pressed", "EditorOutputTab", theme->GetStyleBox("tab_selected_hover", "TabContainer"));
	theme->SetColor("font_color", "EditorOutputTab", muted);
	theme->SetColor("font_pressed_color", "EditorOutputTab", text);
	theme->SetColor("font_hover_pressed_color", "EditorOutputTab", text);
	theme->SetConstant("minimum_height", "EditorOutputTab", 33);
	auto menuItem = box(Color::Transparent, Color::Transparent, 0.f, 10.f, 3.75f);
	menuItem->SetContentMargin(ThemeStyleBox::Side::Left, 32.f);
	theme->SetStyleBox("normal", "EditorMenuItem", menuItem);
	auto menuHover = box(elevated, Color::Transparent, 0.f, 10.f, 3.75f);
	menuHover->SetContentMargin(ThemeStyleBox::Side::Left, 32.f);
	theme->SetStyleBox("hover", "EditorMenuItem", menuHover);
	theme->SetStyleBox("disabled", "EditorMenuItem", menuItem);
	theme->SetColor("separator_color", "MenuBar", border);
	theme->SetTypeVariation("EditorObjectBar", "PanelContainer");
	theme->SetStyleBox("panel", "EditorObjectBar", box(field, Color::Transparent, 0.f, 7.5f, 3.75f));
	theme->SetTypeVariation("EditorGroupButton", "EditorToolbarButton");
	theme->SetColor("font_color", "EditorGroupButton", text);
	theme->SetTypeVariation("EditorPropertyLabel", "Label");
	theme->SetFontSize("font_size", "EditorPropertyLabel", 14);
	theme->SetColor("font_color", "EditorPropertyLabel", muted);
	for (char const* variation : { "EditorLogTime", "EditorLogInfo", "EditorLogWarning", "EditorLogError" })
	{
		theme->SetTypeVariation(variation, "Label");
		theme->SetFontSize("font_size", variation, 14);
	}

	theme->SetColor("font_color", "EditorLogTime", dim);
	theme->SetColor("font_color", "EditorLogInfo", Color(145, 185, 155));
	theme->SetColor("font_color", "EditorLogWarning", Color(226, 186, 114));
	theme->SetColor("font_color", "EditorLogError", Color(227, 152, 152));
	for (ClassInfo const* info : ClassDatabase::GetRegisteredClasses())
	{
		if (info != nullptr && info->m_apiType == ApiType::Runtime
			&& ClassDatabase::IsSubclassOf(info->m_className, "Node"))
		{
			theme->SetIcon(info->m_className, "EditorIcons", EditorIcons::GetIconTexture(info->m_className, "Node"));
		}
	}

	theme->SetConstant("guide_width", "Tree", 2);
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

	theme->SetConstant("panel_margin", "Editor", 5);
	theme->SetConstant("content_padding", "Editor", 10);
	theme->SetConstant("spacing", "Editor", 8);
	theme->SetConstant("section_height", "Editor", 30);
	theme->SetConstant("property_label_width", "Editor", 113);
	theme->SetConstant("action_slot", "Editor", 23);
	theme->SetConstant("color_height", "Editor", 28);
	theme->SetConstant("viewport_min_width", "Editor", 280);
	theme->SetConstant("viewport_min_height", "Editor", 200);
	theme->SetConstant("viewport_header_height", "Editor", 33);
	theme->SetConstant("viewport_toolbar_height", "Editor", 38);
	theme->SetConstant("scene_title_width", "Editor", 60);
	theme->SetConstant("ruler_width", "ViewportPanel", 23);
	theme->SetConstant("brand_width", "Editor", 130);
	theme->SetConstant("beam_max_width", "Editor", 900);
	theme->SetConstant("beam_center_gap", "Editor", 50);
	theme->SetConstant("status_spacing", "Editor", 15);
	theme->SetConstant("popup_height", "Editor", 250);
	theme->SetConstant("popup_screen_margin", "Editor", 40);
	theme->SetConstant("separator_height", "Editor", 18);
	theme->SetConstant("children_guide_width", "Tree", 1);
	theme->SetColor("children_guide_color", "Tree", border);
	theme->SetConstant("icon_separation", "LineEdit", 8);
	theme->SetConstant("check_size", "MenuBar", 15);
	theme->SetConstant("item_padding", "MenuBar", 10);
	theme->SetConstant("shortcut_separation", "MenuBar", 25);
	theme->SetConstant("separator_width", "MenuBar", 1);
	theme->SetTypeVariation("EditorOutputFilter", "EditorPanelAction");
	theme->SetStyleBox("normal", "EditorOutputFilter", box(field, Color::Transparent, 0.f, 3.75f, 2.5f));
	theme->SetStyleBox("pressed", "EditorOutputFilter", box(elevated, Color::Transparent, 0.f, 3.75f, 2.5f));
	theme->SetStyleBox("hover_pressed", "EditorOutputFilter", box(strong, Color::Transparent, 0.f, 3.75f, 2.5f));
	theme->SetTypeVariation("EditorLogArea", "PanelContainer");
	theme->SetStyleBox("panel", "EditorLogArea", box(field, Color::Transparent, 0.f, 5.f, 5.f));
	theme->SetColor("background_color", "EditorLogScroll", field);
	theme->SetTypeVariation("EditorLogScroll", "ScrollContainer");
	auto logFont = JetBrainsMonoFont::Create();
	for (char const* variation : { "EditorLogTime", "EditorLogInfo", "EditorLogWarning", "EditorLogError" })
	{
		theme->SetFont("font", variation, logFont);
	}

	theme->SetColor("font_color", "EditorLogInfo", text);
	theme->SetColor("font_color", "MenuBarButton", text);
	theme->SetColor("font_hover_color", "MenuBarButton", text);
	theme->SetConstant("project_popup_width", "Editor", 650);
	theme->SetConstant("project_popup_height", "Editor", 275);
	theme->SetConstant("warning_popup_width", "Editor", 700);
	theme->SetConstant("warning_popup_height", "Editor", 225);
	theme->SetConstant("create_popup_height", "Editor", 300);
	theme->SetConstant("node_popup_width", "Editor", 700);
	theme->SetConstant("node_popup_height", "Editor", 600);
	theme->SetTypeVariation("SpinBoxStep", "Button");
	theme->SetFontSize("font_size", "SpinBoxStep", 12);
	theme->SetTypeVariation("SpinBoxError", "Label");
	theme->SetFontSize("font_size", "SpinBoxError", 11);
	return theme;
}

Ref<Theme> EditorThemeManager::CreateScaledTheme(Ref<Theme> const& baseTheme, float scale)
{
	if (!baseTheme.IsValid())
	{
		return {};
	}

	if (!std::isfinite(scale) || scale <= 0.f)
	{
		scale = 1.25f;
	}

	auto result = baseTheme->Duplicate();
	auto dimension = [scale](int value, bool thin = false)
	{
		double const scaled = static_cast<double>(value) * scale;
		double const rounded = thin ? std::floor(scaled) : std::round(scaled);
		int const minimum = thin && value > 0 ? 1 : 0;
		return static_cast<int>(std::clamp(rounded, static_cast<double>(minimum), static_cast<double>(std::numeric_limits<int>::max())));
	};

	if (baseTheme->HasDefaultFontSize())
	{
		result->SetDefaultFontSize(std::max(1, dimension(baseTheme->GetDefaultFontSize())));
	}

	for (auto const& type : baseTheme->GetTypeList(Theme::DataType::FontSize))
	{
		for (auto const& name : baseTheme->GetItemList(Theme::DataType::FontSize, type))
		{
			result->SetFontSize(name, type, std::max(1, dimension(baseTheme->GetFontSize(name, type))));
		}
	}

	// Only these constants represent editor dimensions; switches and counts retain their values.
	// e.g. A user-defined mode constant is copied even when its value happens to be 20.
	static std::pair<char const*, char const*> const dimensions[] = {
		{ "Editor", "node_popup_height" },
		{ "Editor", "node_popup_width" },
		{ "Editor", "create_popup_height" },
		{ "Editor", "warning_popup_height" },
		{ "Editor", "warning_popup_width" },
		{ "Editor", "project_popup_height" },
		{ "Editor", "project_popup_width" },
		{ "BoxContainer", "separation" },
		{ "Button", "icon_separation" },
		{ "Button", "icon_size" },
		{ "Button", "minimum_height" },
		{ "Editor", "action_slot" },
		{ "Editor", "beam_center_gap" },
		{ "Editor", "beam_max_width" },
		{ "Editor", "brand_width" },
		{ "Editor", "color_height" },
		{ "Editor", "content_padding" },
		{ "Editor", "panel_margin" },
		{ "Editor", "popup_height" },
		{ "Editor", "popup_screen_margin" },
		{ "Editor", "property_label_width" },
		{ "ViewportPanel", "ruler_width" },
		{ "Editor", "scene_title_width" },
		{ "Editor", "section_height" },
		{ "Editor", "separator_height" },
		{ "Editor", "spacing" },
		{ "Editor", "status_bar_height" },
		{ "Editor", "status_spacing" },
		{ "Editor", "top_bar_height" },
		{ "Editor", "viewport_header_height" },
		{ "Editor", "viewport_min_height" },
		{ "Editor", "viewport_min_width" },
		{ "Editor", "viewport_toolbar_height" },
		{ "EditorOutputTab", "minimum_height" },
		{ "EditorPanelAction", "minimum_height" },
		{ "EditorToolbarButton", "minimum_height" },
		{ "Label", "line_spacing" },
		{ "LineEdit", "caret_width" },
		{ "LineEdit", "icon_separation" },
		{ "LineEdit", "minimum_height" },
		{ "MarginContainer", "margin_bottom" },
		{ "MarginContainer", "margin_left" },
		{ "MarginContainer", "margin_right" },
		{ "MarginContainer", "margin_top" },
		{ "MenuBar", "check_size" },
		{ "MenuBar", "item_padding" },
		{ "MenuBar", "minimum_height" },
		{ "MenuBar", "popup_minimum_width" },
		{ "MenuBar", "separator_width" },
		{ "MenuBar", "shortcut_separation" },
		{ "ScrollContainer", "grabber_margin" },
		{ "ScrollContainer", "grabber_minimum_size" },
		{ "ScrollContainer", "scroll_step" },
		{ "ScrollContainer", "scrollbar_width" },
		{ "SplitContainer", "grabber_length" },
		{ "SplitContainer", "grabber_separation" },
		{ "SplitContainer", "grabber_width" },
		{ "SplitContainer", "separation" },
		{ "TabContainer", "drop_mark_width" },
		{ "TabContainer", "page_separation" },
		{ "TabContainer", "tab_height" },
		{ "TabContainer", "tab_minimum_width" },
		{ "TabContainer", "tab_separation" },
		{ "Tree", "children_guide_width" },
		{ "Tree", "fold_size" },
		{ "Tree", "guide_width" },
		{ "Tree", "icon_separation" },
		{ "Tree", "icon_size" },
		{ "Tree", "indent" },
		{ "Tree", "row_height" },
	};
	for (auto const& item : dimensions)
	{
		if (baseTheme->HasConstant(item.second, item.first))
		{
			bool const thin = (std::string(item.first) == "LineEdit" && std::string(item.second) == "caret_width")
				|| (std::string(item.first) == "MenuBar" && std::string(item.second) == "separator_width")
				|| (std::string(item.first) == "SplitContainer" && std::string(item.second) == "grabber_width")
				|| (std::string(item.first) == "TabContainer" && std::string(item.second) == "drop_mark_width")
				|| (std::string(item.first) == "Tree" && std::string(item.second) == "children_guide_width")
				|| (std::string(item.first) == "Tree" && std::string(item.second) == "guide_width");
			result->SetConstant(item.second, item.first, dimension(baseTheme->GetConstant(item.second, item.first), thin));
		}
	}

	std::set<ThemeStyleBox*> scaledBoxes;
	for (auto const& type : result->GetTypeList(Theme::DataType::StyleBox))
	{
		for (auto const& name : result->GetItemList(Theme::DataType::StyleBox, type))
		{
			auto box = result->GetStyleBox(name, type);
			if (!scaledBoxes.insert(box.Get()).second)
			{
				continue;
			}

			for (int i = 0; i < 4; ++i)
			{
				auto const side = static_cast<ThemeStyleBox::Side>(i);
				float const border = box->GetBorderWidth(side);
				box->SetContentMargin(side, box->GetContentMargin(side) * scale);
				box->SetBorderWidth(side, border > 0.f ? std::max(1.f, std::floor(border * scale)) : 0.f);
			}
		}
	}

	return result;
}
