#pragma once

#include "MingEngine/Core/Math/AABB2.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Render/Color.hpp"

#include <map>

class ThemeStyleBox : public Resource
{
	MCLASS(ThemeStyleBox, Resource);

public:
	enum class Side
	{
		Left,
		Top,
		Right,
		Bottom
	};

public:
	Color   GetBackgroundColor() const { return m_backgroundColor; }
	Color   GetBorderColor() const { return m_borderColor; }
	float   GetBorderWidth(Side side) const;
	float   GetContentMargin(Side side) const;
	Vector2 GetMinimumSize() const;
	AABB2   GetContentRect(AABB2 const& rect) const;

	void SetBackgroundColor(Color const& color);
	void SetBorderColor(Color const& color);
	void SetBorderWidth(Side side, float width);
	void SetContentMargin(Side side, float margin);
	bool CopyFrom(Resource&& other) override;

	// Keep weak, counted subscriptions separate from the Ref that owns this resource.
	// e.g. A Theme using one box for normal and hover subscribes twice and releases twice.
	void AddUser(ObjectID user);
	void RemoveUser(ObjectID user);

protected:
	static void BindMethods() {}

private:
	void EmitChanged();

private:
	Color                   m_backgroundColor   = Color::Transparent;
	Color                   m_borderColor       = Color::Transparent;
	float                   m_borderWidths[4]   = {};
	float                   m_contentMargins[4] = {};
	std::map<ObjectID, int> m_users;
};
