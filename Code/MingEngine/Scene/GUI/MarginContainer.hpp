#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"
#include <optional>

class MarginContainer : public Container
{
	MCLASS(MarginContainer, Container);

private:
	struct ThemeCache
	{
		int margin_left = 0;
		int margin_top = 0;
		int margin_right = 0;
		int margin_bottom = 0;
	};

public:
	Vector2 GetInternalMinimumSize() const override;

	float GetMarginLeft() const;
	float GetMarginTop() const;
	float GetMarginRight() const;
	float GetMarginBottom() const;

	void SetMargins(float left, float top, float right, float bottom);

	void SetMarginLeft(float margin) { SetMargin(0, margin); }
	void SetMarginTop(float margin) { SetMargin(1, margin); }
	void SetMarginRight(float margin) { SetMargin(2, margin); }
	void SetMarginBottom(float margin) { SetMargin(3, margin); }
	void RemoveMarginsOverride();

protected:
	static void BindMethods();

	void OnNotification(int notification);

private:
	void SetMargin(int side, float margin);

private:
	std::optional<float> m_marginOverrides[4];
	ThemeCache m_themeCache;
};
