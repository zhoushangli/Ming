#include "MingEngine/Scene/3D/Mesh3D.hpp"

#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"

void Mesh3D::BindMethods()
{
	ClassDatabase::BindMethod("SetMesh", &Mesh3D::SetMesh);
	ClassDatabase::BindMethod("GetMesh", &Mesh3D::GetMesh);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::ObjectPtr,
			"mesh",
			PropertyInfo::Hint::ResourceType,
			MeshResource::GetStaticClassName(),
			PropertyInfo::UsageFlags::Default),
		"SetMesh",
		"GetMesh");
}

void Mesh3D::OnProcess([[maybe_unused]] float deltaSeconds) {}

AABB3 Mesh3D::GetLocalBounds() const { return AABB3(); }

bool Mesh3D::IsEmpty() const { return m_meshResource == nullptr || m_meshResource->IsEmpty(); }

void Mesh3D::SetUseMaterialTextures(bool useMaterialTextures) { m_useMaterialTextures = useMaterialTextures; }

void Mesh3D::SetMesh(Variant meshResource)
{
	Ref<MeshResource> mesh = meshResource;
	if (!mesh.IsValid())
	{
		m_meshResource = nullptr;
		return;
	}

	m_meshResource = mesh;
}

Variant Mesh3D::GetMesh() const { return m_meshResource; }

RenderRequest Mesh3D::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_meshResource == nullptr || m_meshResource->IsEmpty())
	{
		return request;
	}

	request.m_pass         = RenderRequestPass::Opaque;
	request.m_modelToWorld = GetWorldTransform();
	request.m_tint         = Rgba8::White;
	request.m_vertexBuffer = m_meshResource->m_vertexBuffer;
	request.m_indexBuffer  = m_meshResource->m_indexBuffer;
	request.m_textures[SurfaceTextureSlot::Diffuse] =
		m_meshResource->m_textureResources.size() > 0 ? m_meshResource->m_textureResources[0]->GetGPUTexture() : nullptr;
	request.m_shader         = g_engine->m_renderer->CreateOrGetShader("res://Shaders/DefaultLit.hlsl");
	request.m_blendMode      = BlendMode::OPAQUE;
	request.m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	request.m_samplerMode    = SamplerMode::POINT_CLAMP;
	return request;
}
