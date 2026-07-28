#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

class VisualizeInstance3D : public Node3D
{
	MCLASS(VisualizeInstance3D, Node3D);

public:
	VisualizeInstance3D()           = default;
	~VisualizeInstance3D() override = default;

	void SetVisible(bool visible);
	bool GetVisible() const;

	virtual RenderRequest SubmitRenderRequest() const = 0;

protected:
	void        OnNotification(int notification);
	static void BindMethods() {};

public:
	bool m_isVisible = true;
};
