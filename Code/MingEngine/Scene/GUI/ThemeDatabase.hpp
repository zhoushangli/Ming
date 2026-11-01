#pragma once

#include "MingEngine/Scene/Resource/Theme.hpp"

#include <functional>
#include <set>
#include <type_traits>

class Control;
class Node;

struct ThemeItemInfo
{
	Theme::DataType m_dataType = Theme::DataType::Color;
	std::string     m_className;
	std::string     m_name;
};

struct ThemeItemBind
{
	ThemeItemInfo                 m_info;
	std::function<void(Control*)> m_setter;
};

// Bind a same-named item to this class's private ThemeCache m_themeCache; the field type must match.
// e.g. BIND_THEME_ITEM(Theme::DataType::Color, Button, font_color) binds the "font_color" theme item.
#define BIND_THEME_ITEM(dataType, className, memberName)                                                               \
	ThemeDatabase::BindThemeItem(                                                                                      \
		dataType,                                                                                                     \
		className::GetStaticClassName(),                                                                               \
		#memberName,                                                                                                   \
		[](Control* control)                                                                                           \
		{                                                                                                              \
			auto* instance = static_cast<className*>(control);                                                         \
			static_assert(                                                                                             \
				std::is_same_v<decltype(instance->m_themeCache), className::ThemeCache>,                               \
				"Theme binding requires this class's ThemeCache m_themeCache.");                                       \
			static_assert(                                                                                             \
				std::is_same_v<                                                                                        \
					std::remove_reference_t<decltype(instance->m_themeCache.memberName)>,                              \
					decltype(instance->GetThemeItem<dataType>(#memberName))>,                                          \
				"Theme cache field type must match the registered data category.");                                    \
			instance->m_themeCache.memberName = instance->GetThemeItem<dataType>(#memberName);                         \
		})

class ThemeDatabase
{
public:
	static void Startup();
	static void Shutdown();

	// Register during BindMethods; instance cache writes happen only after full construction.
	// e.g. Button::BindMethods registers callbacks later run from Control's post-initialize notification.
	static void BindThemeItem(
		Theme::DataType               type,
		std::string const&            className,
		std::string const&            name,
		std::function<void(Control*)> setter);
	static std::vector<ThemeItemInfo> GetThemeItems(std::string const& className);
	static std::vector<ThemeItemInfo> GetAllThemeItems(std::string const& className);
	static ThemeItemInfo const*       FindThemeItem(
		std::string const& className, Theme::DataType type, std::string const& name);
	static void UpdateClassInstanceItems(Control* control);

	static Ref<Theme> GetDefaultTheme();
	static void       SetDefaultTheme(Ref<Theme> const& theme);
	static void       RegisterControl(Control* control);
	static void       UnregisterControl(Control* control);

	// Context entries are ordered fallback sources; the engine default is appended during lookup.
	// e.g. SetThemeContext(editorRoot, { editorTheme }) isolates editor defaults from a scene context.
	static void SetThemeContext(Node* root, std::vector<Ref<Theme>> const& themes);
	static void ClearThemeContext(Node* root);
	static std::vector<Ref<Theme>> GetThemeContext(Node const* node);
	static void RefreshThemeContexts(Node* root);

private:
	static std::map<std::string, std::vector<ThemeItemBind>> m_items;
	static std::set<ObjectID>                                m_controls;
	static Ref<Theme>                                        m_defaultTheme;
	static std::map<ObjectID, std::vector<Ref<Theme>>> m_contexts;
};
