#include "MingEngine/Scene/3D/Mesh3D.hpp"

#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

Mesh3D::Mesh3D(std::string const& modelFilePath, float scale) : VisualizeInstance3D()
{
	m_meshResource = MeshResource::CreateOrGetMesh(modelFilePath, scale);
}

Mesh3D::~Mesh3D() {}

void Mesh3D::OnProcess([[maybe_unused]] float deltaSeconds) {}

AABB3 Mesh3D::GetLocalBounds() const { return m_meshResource != nullptr ? m_meshResource->m_localBounds : AABB3(); }

bool Mesh3D::IsEmpty() const { return m_meshResource == nullptr || m_meshResource->IsEmpty(); }

void Mesh3D::SetUseMaterialTextures(bool useMaterialTextures) { m_useMaterialTextures = useMaterialTextures; }

RenderRequest Mesh3D::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_meshResource == nullptr || m_meshResource->m_meshData.m_vertices.empty())
	{
		return request;
	}

	request.m_pass         = RenderRequestPass::Opaque;
	request.m_modelToWorld = GetWorldTransform();
	request.m_tint         = Rgba8::White;
	request.m_vertexBuffer = m_meshResource->m_vertexBuffer;
	request.m_indexBuffer  = m_meshResource->m_indexBuffer;
	request.m_textures[SurfaceTextureSlot::Diffuse] =
		m_useMaterialTextures ? m_meshResource->m_meshData.m_texture : nullptr;
	request.m_shader         = g_engine->m_renderer->CreateOrGetShader("res://Shaders/DefaultLit.hlsl");
	request.m_blendMode      = BlendMode::OPAQUE;
	request.m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	request.m_samplerMode    = SamplerMode::POINT_CLAMP;
	return request;
}
