#include "MingEngine/Scene/GUI/ThemeDatabase.hpp"

#include "MingEngine/Core/ErrorWarningAssert.hpp"
#include "MingEngine/Scene/GUI/Control.hpp"
#include "MingEngine/Scene/GUI/DefaultTheme.hpp"

std::map<std::string, std::vector<ThemeItemBind>> ThemeDatabase::m_items;
std::set<ObjectID>                                ThemeDatabase::m_controls;
Ref<Theme>                                        ThemeDatabase::m_defaultTheme;

std::map<ObjectID, std::vector<Ref<Theme>>> ThemeDatabase::m_contexts;

void ThemeDatabase::Startup()
{
	SetDefaultTheme(CreateDefaultTheme());
}

void ThemeDatabase::Shutdown()
{
	GUARANTEE_OR_DIE(m_controls.empty(), "ThemeDatabase: destroy Controls before shutting down theme resources.");
	m_contexts.clear();
	m_defaultTheme = nullptr;
	// Bindings live for the process, matching MCLASS::InitializeClass's one-time registration.
	// e.g. Startup after Shutdown replaces resources without losing callbacks that cannot be registered twice.
}

void ThemeDatabase::BindThemeItem(
	Theme::DataType type, std::string const& className, std::string const& name, std::function<void(Control*)> setter)
{
	auto& items = m_items[className];
	for (auto const& item : items)
	{
		GUARANTEE_OR_DIE(
			item.m_info.m_dataType != type || item.m_info.m_name != name,
			"ThemeDatabase: duplicate theme item in the same class.");
	}

	items.push_back({ { type, className, name }, std::move(setter) });
}

std::vector<ThemeItemInfo> ThemeDatabase::GetThemeItems(std::string const& className)
{
	std::vector<ThemeItemInfo> result;
	auto                       found = m_items.find(className);
	if (found != m_items.end())
	{
		for (auto const& bind : found->second)
		{
			result.push_back(bind.m_info);
		}
	}

	return result;
}

std::vector<ThemeItemInfo> ThemeDatabase::GetAllThemeItems(std::string const& className)
{
	std::vector<ThemeItemInfo> result;
	std::string                current = className;
	while (!current.empty())
	{
		for (auto const& item : GetThemeItems(current))
		{
			bool exists = false;
			for (auto const& previous : result)
			{
				exists |= previous.m_dataType == item.m_dataType && previous.m_name == item.m_name;
			}

			if (!exists)
			{
				result.push_back(item);
			}
		}

		auto* info = ClassDatabase::GetClassInfo(current);
		current    = info != nullptr ? info->m_parentClassName : std::string();
	}

	return result;
}

ThemeItemInfo const* ThemeDatabase::FindThemeItem(
	std::string const& className, Theme::DataType type, std::string const& name)
{
	std::string current = className;
	while (!current.empty())
	{
		auto found = m_items.find(current);
		if (found != m_items.end())
		{
			for (auto const& bind : found->second)
			{
				if (bind.m_info.m_dataType == type && bind.m_info.m_name == name)
				{
					return &bind.m_info;
				}
			}
		}

		auto* info = ClassDatabase::GetClassInfo(current);
		current    = info != nullptr ? info->m_parentClassName : std::string();
	}

	return nullptr;
}

void ThemeDatabase::UpdateClassInstanceItems(Control* control)
{
	// Each callback casts to its declaring class, preserving separate inherited caches.
	// e.g. An OptionButton refresh runs Button's bindings against Button::m_themeCache.
	std::string current = control->GetClassName();
	while (!current.empty())
	{
		auto found = m_items.find(current);
		if (found != m_items.end())
		{
			for (auto const& bind : found->second)
			{
				bind.m_setter(control);
			}
		}

		auto* info = ClassDatabase::GetClassInfo(current);
		current    = info != nullptr ? info->m_parentClassName : std::string();
	}
}

Ref<Theme> ThemeDatabase::GetDefaultTheme()
{
	return m_defaultTheme;
}

void ThemeDatabase::SetDefaultTheme(Ref<Theme> const& theme)
{
	if (m_defaultTheme == theme)
	{
		return;
	}

	m_defaultTheme      = theme;
	auto const controls = m_controls;
	for (ObjectID id : controls)
	{
		auto* control = ObjectDatabase::GetInstance<Control>(id);
		if (control != nullptr && !control->IsPreDeleting())
		{
			control->RefreshTheme();
		}
	}
}

void ThemeDatabase::RegisterControl(Control* control)
{
	m_controls.insert(control->GetObjectID());
}

void ThemeDatabase::UnregisterControl(Control* control)
{
	m_controls.erase(control->GetObjectID());
}

void ThemeDatabase::SetThemeContext(Node* root, std::vector<Ref<Theme>> const& themes)
{
	if (root == nullptr || root->IsPreDeleting())
	{
		return;
	}

	m_contexts[root->GetObjectID()] = themes;
	RefreshThemeContexts(root);
}

void ThemeDatabase::ClearThemeContext(Node* root)
{
	if (root != nullptr && m_contexts.erase(root->GetObjectID()) != 0)
	{
		RefreshThemeContexts(root);
	}
}

std::vector<Ref<Theme>> ThemeDatabase::GetThemeContext(Node const* node)
{
	for (Node const* current = node; current != nullptr; current = current->GetParent())
	{
		auto found = m_contexts.find(current->GetObjectID());
		if (found != m_contexts.end())
		{
			return found->second;
		}
	}

	return {};
}

void ThemeDatabase::RefreshThemeContexts(Node* root)
{
	if (root == nullptr || root->IsPreDeleting())
	{
		return;
	}

	if (auto* control = dynamic_cast<Control*>(root))
	{
		control->RefreshTheme();
	}

	auto const children = root->GetChildren();
	for (Node* child : children)
	{
		RefreshThemeContexts(child);
	}
}
