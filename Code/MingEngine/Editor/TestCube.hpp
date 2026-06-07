#pragma once

#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"

#include "MingEngine/Engine/Render/Vertex.hpp"

#include <vector>

class TestCube : public VisualizeInstance3D
{
	CLASS(TestCube, Node3D);

public:
	TestCube();
	~TestCube() override;

protected:
	void OnReady() override;
	void OnExitTree() override;
	RenderRequest SubmitRenderRequest() const override;

private:
	std::vector<Vertex> m_renderVerts;
	VertexBuffer*       m_renderVertexBuffer;
};
