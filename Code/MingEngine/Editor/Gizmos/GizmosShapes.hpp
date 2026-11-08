#pragma once

#include "MingEngine/Scene/3D/VisualInstance3D.hpp"

#include "MingEngine/Core/Render/Vertex.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"
#include "MingEngine/Engine/Render/RenderContext.hpp"

#include <vector>

class EditorGizmoVisual3D : public VisualInstance3D
{
	MCLASS(EditorGizmoVisual3D, VisualInstance3D)

public:
	EditorGizmoVisual3D()           = default;
	~EditorGizmoVisual3D() override = default;

	void SetRenderPriority(int priority);

	void UploadGeometry(std::vector<Vertex> const& vertices);
	void UploadGeometry(
		char const*       shaderName,
		std::string_view  shaderSource,
		RenderRequestPass pass  = RenderRequestPass::EditorOverlay,
		DepthMode         depth = DepthMode::READ_ONLY_ALWAYS);
	void SetGizmoColor(Color const& color);
	void SetGizmoTexture(Ref<TextureResource> const& texture);

protected:
	std::vector<Vertex>       m_verts;
	std::vector<unsigned int> m_indices;
	Ref<MeshResource>         m_mesh;
};

class EditorWorldGrid3D : public EditorGizmoVisual3D
{
public:
	EditorWorldGrid3D();
};

// TODO: Current Shader looks strange when get close to the axsis, it will scale up when you
// look up/down, and scale down when you look horizontally
class EditorWorldAxis3D : public EditorGizmoVisual3D
{
public:
	EditorWorldAxis3D(Vector3 const& axisStart, Vector3 const& axisEnd, Color const& color);
};
