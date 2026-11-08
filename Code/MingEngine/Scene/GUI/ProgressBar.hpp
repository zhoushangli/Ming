#pragma once

#include "MingEngine/Scene/GUI/Control.hpp"

class ProgressBar : public Control
{
	MCLASS(ProgressBar, Control);

private:
	struct ThemeCache
	{
		Ref<ThemeStyleBox> background;
		Ref<ThemeStyleBox> fill;
		int minimum_height = 12;
	};

public:
	ProgressBar();

	float GetMinValue() const { return m_minValue; }
	float GetMaxValue() const { return m_maxValue; }
	float GetValue() const { return m_value; }
	bool IsIndeterminate() const { return m_indeterminate; }
	Vector2 GetInternalMinimumSize() const override;

	void SetMinValue(float value);
	void SetMaxValue(float value);
	void SetValue(float value);
	void SetIndeterminate(bool enabled);

protected:
	static void BindMethods();

private:
	void OnProcess(float deltaSeconds) override;
	void OnNotification(int notification);

private:
	ThemeCache m_themeCache;
	float m_minValue = 0.f;
	float m_maxValue = 100.f;
	float m_value = 0.f;
	float m_phase = 0.f;
	bool m_indeterminate = false;
};
