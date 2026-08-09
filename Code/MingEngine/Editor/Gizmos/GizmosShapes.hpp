#pragma once

#include "MingEngine/Scene/3D/VisualInstance3D.hpp"

#include "MingEngine/Core/Render/Vertex.hpp"

#include <vector>

class VertexBuffer;

class EditorGizmoVisual3D : public VisualInstance3D
{
	MCLASS(EditorGizmoVisual3D, VisualInstance3D)

public:
	EditorGizmoVisual3D() = default;
	~EditorGizmoVisual3D() override;

protected:
	RenderRequest SubmitRenderRequest() const override;

protected:
	std::vector<Vertex> m_verts;
	VertexBuffer*       m_vertexBuffer = nullptr;
};

class EditorWorldGrid3D : public EditorGizmoVisual3D
{
public:
	EditorWorldGrid3D();

protected:
	RenderRequest SubmitRenderRequest() const override;
};

// TODO: Current Shader looks strange when get close to the axsis, it will scale up when you
// look up/down, and scale down when you look horizontally
class EditorWorldAxis3D : public EditorGizmoVisual3D
{
public:
	EditorWorldAxis3D(Vector3 const& axisStart, Vector3 const& axisEnd, Color const& color);

protected:
	RenderRequest SubmitRenderRequest() const override;
};
