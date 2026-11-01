#include "MingEngine/Scene/Resource/Theme.hpp"

#include "MingEngine/Scene/GUI/Control.hpp"

Theme::~Theme()
{
	ReleaseStyleBoxes();
}

Color Theme::GetColor(std::string const& name, std::string const& type) const
{
	auto group = m_colorItems.find(type);
	if (group != m_colorItems.end())
	{
		auto item = group->second.find(name);
		if (item != group->second.end())
		{
			return item->second;
		}
	}

	return Color::Transparent;
}

bool Theme::HasColor(std::string const& name, std::string const& type) const
{
	auto group = m_colorItems.find(type);
	return group != m_colorItems.end() && group->second.find(name) != group->second.end();
}

void Theme::SetColor(std::string const& name, std::string const& type, Color const& value)
{
	if (HasColor(name, type) && GetColor(name, type) == value)
	{
		return;
	}

	m_colorItems[type][name] = value;
	EmitChanged();
}

void Theme::RemoveColor(std::string const& name, std::string const& type)
{
	auto group = m_colorItems.find(type);
	if (group == m_colorItems.end())
	{
		return;
	}

	if (group->second.erase(name) != 0)
	{
		EmitChanged();
	}
}

int Theme::GetConstant(std::string const& name, std::string const& type) const
{
	auto group = m_constantItems.find(type);
	if (group != m_constantItems.end())
	{
		auto item = group->second.find(name);
		if (item != group->second.end())
		{
			return item->second;
		}
	}

	return 0;
}

bool Theme::HasConstant(std::string const& name, std::string const& type) const
{
	auto group = m_constantItems.find(type);
	return group != m_constantItems.end() && group->second.find(name) != group->second.end();
}

void Theme::SetConstant(std::string const& name, std::string const& type, int value)
{
	if (HasConstant(name, type) && GetConstant(name, type) == value)
	{
		return;
	}

	m_constantItems[type][name] = value;
	EmitChanged();
}

void Theme::RemoveConstant(std::string const& name, std::string const& type)
{
	auto group = m_constantItems.find(type);
	if (group == m_constantItems.end())
	{
		return;
	}

	if (group->second.erase(name) != 0)
	{
		EmitChanged();
	}
}

void Theme::SetDefaultFont(Ref<FontResource> const& font)
{
	if (m_defaultFont != font)
	{
		m_defaultFont = font;
		EmitChanged();
	}
}

void Theme::SetDefaultFontSize(int size)
{
	if (size != -1 && size <= 0)
	{
		return;
	}

	if (m_defaultFontSize != size)
	{
		m_defaultFontSize = size;
		EmitChanged();
	}
}

Ref<FontResource> Theme::GetFont(std::string const& name, std::string const& type) const
{
	auto group = m_fontItems.find(type);
	if (group != m_fontItems.end())
	{
		auto item = group->second.find(name);
		if (item != group->second.end())
		{
			return item->second;
		}
	}

	return {};
}

bool Theme::HasFont(std::string const& name, std::string const& type) const
{
	auto group = m_fontItems.find(type);
	return group != m_fontItems.end() && group->second.find(name) != group->second.end();
}

void Theme::SetFont(std::string const& name, std::string const& type, Ref<FontResource> const& value)
{
	if (!value.IsValid())
	{
		RemoveFont(name, type);
		return;
	}

	if (HasFont(name, type) && GetFont(name, type) == value)
	{
		return;
	}

	m_fontItems[type][name] = value;
	EmitChanged();
}

void Theme::RemoveFont(std::string const& name, std::string const& type)
{
	auto group = m_fontItems.find(type);
	if (group == m_fontItems.end())
	{
		return;
	}

	if (group->second.erase(name) != 0)
	{
		EmitChanged();
	}
}

int Theme::GetFontSize(std::string const& name, std::string const& type) const
{
	auto group = m_fontSizeItems.find(type);
	if (group != m_fontSizeItems.end())
	{
		auto item = group->second.find(name);
		if (item != group->second.end())
		{
			return item->second;
		}
	}

	return 16;
}

bool Theme::HasFontSize(std::string const& name, std::string const& type) const
{
	auto group = m_fontSizeItems.find(type);
	return group != m_fontSizeItems.end() && group->second.find(name) != group->second.end();
}

void Theme::SetFontSize(std::string const& name, std::string const& type, int value)
{
	if (value <= 0)
	{
		return;
	}

	if (HasFontSize(name, type) && GetFontSize(name, type) == value)
	{
		return;
	}

	m_fontSizeItems[type][name] = value;
	EmitChanged();
}

void Theme::RemoveFontSize(std::string const& name, std::string const& type)
{
	auto group = m_fontSizeItems.find(type);
	if (group == m_fontSizeItems.end())
	{
		return;
	}

	if (group->second.erase(name) != 0)
	{
		EmitChanged();
	}
}

Ref<TextureResource> Theme::GetIcon(std::string const& name, std::string const& type) const
{
	auto group = m_iconItems.find(type);
	if (group != m_iconItems.end())
	{
		auto item = group->second.find(name);
		if (item != group->second.end())
		{
			return item->second;
		}
	}

	return {};
}

bool Theme::HasIcon(std::string const& name, std::string const& type) const
{
	auto group = m_iconItems.find(type);
	return group != m_iconItems.end() && group->second.find(name) != group->second.end();
}

void Theme::SetIcon(std::string const& name, std::string const& type, Ref<TextureResource> const& value)
{
	if (!value.IsValid())
	{
		RemoveIcon(name, type);
		return;
	}

	if (HasIcon(name, type) && GetIcon(name, type) == value)
	{
		return;
	}

	m_iconItems[type][name] = value;
	EmitChanged();
}

void Theme::RemoveIcon(std::string const& name, std::string const& type)
{
	auto group = m_iconItems.find(type);
	if (group == m_iconItems.end())
	{
		return;
	}

	if (group->second.erase(name) != 0)
	{
		EmitChanged();
	}
}

Ref<ThemeStyleBox> Theme::GetStyleBox(std::string const& name, std::string const& type) const
{
	auto group = m_styleBoxItems.find(type);
	if (group != m_styleBoxItems.end())
	{
		auto item = group->second.find(name);
		if (item != group->second.end())
		{
			return item->second;
		}
	}

	return {};
}

bool Theme::HasStyleBox(std::string const& name, std::string const& type) const
{
	auto group = m_styleBoxItems.find(type);
	return group != m_styleBoxItems.end() && group->second.find(name) != group->second.end();
}

void Theme::SetStyleBox(std::string const& name, std::string const& type, Ref<ThemeStyleBox> const& value)
{
	if (!value.IsValid())
	{
		RemoveStyleBox(name, type);
		return;
	}

	if (HasStyleBox(name, type) && GetStyleBox(name, type) == value)
	{
		return;
	}

	Ref<ThemeStyleBox> previous = GetStyleBox(name, type);
	if (previous.IsValid())
	{
		previous->RemoveUser(GetObjectID());
	}

	value->AddUser(GetObjectID());
	m_styleBoxItems[type][name] = value;
	EmitChanged();
}

void Theme::RemoveStyleBox(std::string const& name, std::string const& type)
{
	auto group = m_styleBoxItems.find(type);
	if (group == m_styleBoxItems.end())
	{
		return;
	}

	auto item = group->second.find(name);
	if (item != group->second.end())
	{
		item->second->RemoveUser(GetObjectID());
	}

	if (group->second.erase(name) != 0)
	{
		EmitChanged();
	}
}

bool Theme::SetTypeVariation(std::string const& variation, std::string const& baseType)
{
	if (variation.empty() || baseType.empty())
	{
		return false;
	}

	std::set<std::string> visited{ variation };
	std::string           current = baseType;
	while (!current.empty())
	{
		if (!visited.insert(current).second)
		{
			return false;
		}

		current = GetTypeVariationBase(current);
	}

	if (GetTypeVariationBase(variation) != baseType)
	{
		m_variations[variation] = baseType;
		EmitChanged();
	}

	return true;
}

void Theme::RemoveTypeVariation(std::string const& variation)
{
	if (m_variations.erase(variation) != 0)
	{
		EmitChanged();
	}
}

std::string Theme::GetTypeVariationBase(std::string const& variation) const
{
	auto found = m_variations.find(variation);
	return found != m_variations.end() ? found->second : std::string();
}

void Theme::AddUser(ObjectID user)
{
	m_users.insert(user);
}

void Theme::RemoveUser(ObjectID user)
{
	m_users.erase(user);
}

void Theme::EmitChanged()
{
	m_pendingChange = true;
	if (m_notifying)
	{
		return;
	}

	Ref<Theme> keepAlive(GetRefCount() > 0 ? this : nullptr);
	m_notifying = true;
	while (m_pendingChange)
	{
		m_pendingChange  = false;
		auto const users = m_users;
		for (ObjectID id : users)
		{
			if (m_users.find(id) == m_users.end())
			{
				continue;
			}

			Control* control = ObjectDatabase::GetInstance<Control>(id);
			if (control == nullptr || control->IsPreDeleting() || control->IsQueuedForDeletion())
			{
				m_users.erase(id);
				continue;
			}

			control->RefreshTheme();
		}
	}

	m_notifying = false;
}

void Theme::ReleaseStyleBoxes()
{
	for (auto const& group : m_styleBoxItems)
	{
		for (auto const& item : group.second)
		{
			item.second->RemoveUser(GetObjectID());
		}
	}
}

bool Theme::CopyFrom(Resource&& other)
{
	auto* theme = dynamic_cast<Theme*>(&other);
	if (theme == nullptr || theme == this)
	{
		return false;
	}

	ReleaseStyleBoxes();
	theme->ReleaseStyleBoxes();
	MoveBaseFrom(std::move(other));
	m_defaultFont = theme->m_defaultFont;
	m_defaultFontSize = theme->m_defaultFontSize;
	theme->m_defaultFont = nullptr;
	theme->m_defaultFontSize = -1;
	m_colorItems = std::move(theme->m_colorItems);
	theme->m_colorItems.clear();
	m_constantItems = std::move(theme->m_constantItems);
	theme->m_constantItems.clear();
	m_fontItems = std::move(theme->m_fontItems);
	theme->m_fontItems.clear();
	m_fontSizeItems = std::move(theme->m_fontSizeItems);
	theme->m_fontSizeItems.clear();
	m_iconItems = std::move(theme->m_iconItems);
	theme->m_iconItems.clear();
	m_styleBoxItems = std::move(theme->m_styleBoxItems);
	theme->m_styleBoxItems.clear();
	m_variations = std::move(theme->m_variations);
	theme->m_variations.clear();
	for (auto const& group : m_styleBoxItems)
	{
		for (auto const& item : group.second)
		{
			item.second->AddUser(GetObjectID());
		}
	}

	EmitChanged();
	theme->EmitChanged();
	return true;
}
