#pragma once

#include "MingEngine/Scene/3D/Node3D.hpp"

#include "MingEngine/Engine/Render/RenderContext.hpp"

class VisualizeInstance3D : public Node3D
{
public:
	VisualizeInstance3D()           = default;
	~VisualizeInstance3D() override = default;

	void OnEnterTree() override;
	void OnExitTree() override;

	virtual RenderRequest SubmitRenderRequest() const = 0;
};
