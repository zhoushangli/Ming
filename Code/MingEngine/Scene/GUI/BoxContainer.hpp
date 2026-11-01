#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"
#include <optional>

class BoxContainer : public Container
{
	MCLASS(BoxContainer, Container);

private:
	struct ThemeCache
	{
		int separation = 0;
	};

public:
	explicit BoxContainer(bool vertical = false) : m_vertical(vertical) {}

	Vector2 GetInternalMinimumSize() const override;

	bool  IsVertical() const { return m_vertical; }
	float GetSeparation() const;

	void SetSeparation(float separation);
	void RemoveSeparationOverride();

protected:
	static void BindMethods();

	void OnNotification(int notification);

private:
	bool  m_vertical   = false;
	std::optional<float> m_separationOverride;
	ThemeCache m_themeCache;
};
