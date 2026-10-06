#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"

class BoxContainer : public Container
{
	MCLASS(BoxContainer, Container);

public:
	explicit BoxContainer(bool vertical = false) : m_vertical(vertical) {}

	Vector2 GetMinimumSize() const override;
	bool IsVertical() const { return m_vertical; }
	float GetSeparation() const { return m_separation; }
	void SetSeparation(float separation);

protected:
	static void BindMethods() {}
	void OnNotification(int notification);

private:
	bool m_vertical = false;
	float m_separation = 0.0f;
};
