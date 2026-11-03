#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

class PanelContainer : public Container
{
	MCLASS(PanelContainer, Container);

private:
	struct ThemeCache
	{
		Ref<ThemeStyleBox> panel;
	};

public:
	Vector2 GetInternalMinimumSize() const override;

protected:
	static void BindMethods();
	void        OnNotification(int notification);

private:
	ThemeCache m_themeCache;
};
