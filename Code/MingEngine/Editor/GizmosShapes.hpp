#pragma once

#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"

#include "MingEngine/Engine/Render/Vertex.hpp"

#include <vector>

class VertexBuffer;

class EditorGizmoVisual3D : public VisualizeInstance3D
{
public:
	EditorGizmoVisual3D() = default;
	~EditorGizmoVisual3D() override;

protected:
	RenderRequest SubmitRenderRequest() const override;

	std::vector<Vertex> m_vertices;
	VertexBuffer*       m_vertexBuffer = nullptr;
};

class EditorWorldGrid3D : public EditorGizmoVisual3D
{
public:
	EditorWorldGrid3D();
};

class EditorWorldAxis3D : public EditorGizmoVisual3D
{
public:
	EditorWorldAxis3D();
};
