#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

class MarginContainer : public Container
{
	MCLASS(MarginContainer, Container);

public:
	Vector2 GetMinimumSize() const override;

	float GetMarginLeft() const { return m_marginLeft; }
	float GetMarginTop() const { return m_marginTop; }
	float GetMarginRight() const { return m_marginRight; }
	float GetMarginBottom() const { return m_marginBottom; }

	void SetMargins(float left, float top, float right, float bottom);
	void SetMarginLeft(float margin) { SetMargins(margin, m_marginTop, m_marginRight, m_marginBottom); }
	void SetMarginTop(float margin) { SetMargins(m_marginLeft, margin, m_marginRight, m_marginBottom); }
	void SetMarginRight(float margin) { SetMargins(m_marginLeft, m_marginTop, margin, m_marginBottom); }
	void SetMarginBottom(float margin) { SetMargins(m_marginLeft, m_marginTop, m_marginRight, margin); }

protected:
	static void BindMethods() {}
	void OnNotification(int notification);

private:
	float m_marginLeft = 0.0f;
	float m_marginTop = 0.0f;
	float m_marginRight = 0.0f;
	float m_marginBottom = 0.0f;
};
