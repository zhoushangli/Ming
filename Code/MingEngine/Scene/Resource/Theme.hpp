#pragma once

#include "MingEngine/Scene/Resource/ThemeStyleBox.hpp"
#include "MingEngine/Scene/Resource/FontResource.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"

#include <map>
#include <set>
#include <string>
#include <vector>

class Theme : public Resource
{
	MCLASS(Theme, Resource);

public:
	enum class DataType
	{
		Color,
		Constant,
		Font,
		FontSize,
		Icon,
		StyleBox
	};

private:
	// ItemMap is a map in map
	// The first key is the type of the item, e.g. "Button", "Label", "Panel"
	// The second key is the name of the item, e.g. "hover", "normal", "pressed"
	// e.g. m_colorItems["Button"]["hover"] = Color::Red 
	template <typename T>
	using ItemMap = std::map<std::string, std::map<std::string, T>>;

public:
	~Theme() override;

	Ref<FontResource> GetDefaultFont() const { return m_defaultFont; }
	int GetDefaultFontSize() const { return m_defaultFontSize; }
	bool HasDefaultFont() const { return m_defaultFont.IsValid(); }
	bool HasDefaultFontSize() const { return m_defaultFontSize > 0; }
	void SetDefaultFont(Ref<FontResource> const& font);
	void SetDefaultFontSize(int size);

	Color GetColor(std::string const& name, std::string const& type) const;
	bool  HasColor(std::string const& name, std::string const& type) const;
	void  SetColor(std::string const& name, std::string const& type, Color const& value);
	void  RemoveColor(std::string const& name, std::string const& type);

	int  GetConstant(std::string const& name, std::string const& type) const;
	bool HasConstant(std::string const& name, std::string const& type) const;
	void SetConstant(std::string const& name, std::string const& type, int value);
	void RemoveConstant(std::string const& name, std::string const& type);

	Ref<FontResource> GetFont(std::string const& name, std::string const& type) const;
	bool              HasFont(std::string const& name, std::string const& type) const;
	void              SetFont(std::string const& name, std::string const& type, Ref<FontResource> const& value);
	void              RemoveFont(std::string const& name, std::string const& type);

	int  GetFontSize(std::string const& name, std::string const& type) const;
	bool HasFontSize(std::string const& name, std::string const& type) const;
	void SetFontSize(std::string const& name, std::string const& type, int value);
	void RemoveFontSize(std::string const& name, std::string const& type);

	Ref<TextureResource> GetIcon(std::string const& name, std::string const& type) const;
	bool                 HasIcon(std::string const& name, std::string const& type) const;
	void                 SetIcon(std::string const& name, std::string const& type, Ref<TextureResource> const& value);
	void                 RemoveIcon(std::string const& name, std::string const& type);

	Ref<ThemeStyleBox> GetStyleBox(std::string const& name, std::string const& type) const;
	bool               HasStyleBox(std::string const& name, std::string const& type) const;
	void               SetStyleBox(std::string const& name, std::string const& type, Ref<ThemeStyleBox> const& value);
	void               RemoveStyleBox(std::string const& name, std::string const& type);

	bool        SetTypeVariation(std::string const& variation, std::string const& baseType);
	void        RemoveTypeVariation(std::string const& variation);
	std::string GetTypeVariationBase(std::string const& variation) const;
	bool        CopyFrom(Resource&& other) override;

	void AddUser(ObjectID user);
	void RemoveUser(ObjectID user);
	void EmitChanged();

protected:
	static void BindMethods() {}

private:
	void ReleaseStyleBoxes();

private:
	Ref<FontResource> m_defaultFont;
	int m_defaultFontSize = -1;
	ItemMap<Color>                     m_colorItems;
	ItemMap<int>                       m_constantItems;
	ItemMap<Ref<FontResource>>         m_fontItems;
	ItemMap<int>                       m_fontSizeItems;
	ItemMap<Ref<TextureResource>>      m_iconItems;
	ItemMap<Ref<ThemeStyleBox>>        m_styleBoxItems;
	std::map<std::string, std::string> m_variations;
	// Subscriptions are weak identities; resource items above retain their own strong references.
	// e.g. A deleted Control cannot stay alive merely because it used this Theme.
	std::set<ObjectID> m_users;
	bool               m_notifying     = false;
	bool               m_pendingChange = false;
};
