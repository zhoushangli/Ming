#pragma once

#include "MingEngine/Scene/GUI/Container.hpp"
#include "MingEngine/Scene/Core/Viewport.hpp"

class SubViewportContainer : public Container
{
	MCLASS(SubViewportContainer, Container);

public:
	SubViewportContainer();
	SubViewport* GetSubViewport() const;
	Vector2      ToViewportPosition(Vector2 windowPosition) const;
	bool         GuiInput(InputEvent const& event) override;
	Vector2      GetInternalMinimumSize() const override { return Vector2(1.f, 1.f); }
	void         SetInputCallback(std::function<bool(InputEvent const&)> callback) { m_input = std::move(callback); }

protected:
	static void BindMethods() {}
	void        OnNotification(int notification);

private:
	ObjectID                               m_viewportID = ObjectID::Invalid;
	std::function<bool(InputEvent const&)> m_input;
};
