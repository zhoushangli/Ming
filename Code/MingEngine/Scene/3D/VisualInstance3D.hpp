#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

class VisualInstance3D : public Node3D
{
	MCLASS(VisualInstance3D, Node3D);

public:
	VisualInstance3D() = default;
	~VisualInstance3D() override = default;

	void SetVisible(bool visible);
	bool GetVisible() const;

	virtual RenderRequest SubmitRenderRequest() const = 0;

protected:
	void OnNotification(int notification);
	static void BindMethods() {};

public:
	bool m_isVisible = true;
};
