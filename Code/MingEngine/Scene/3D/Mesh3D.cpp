#include "MingEngine/Scene/3D/Mesh3D.hpp"

#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

RaycastResult3D MeshRaycastObject::IntersectBounds(RaycastQuery3D const& query)
{
	RaycastResult3D hit;

	Matrix4x4 localToWorld = m_mesh->GetWorldTransform();
	Matrix4x4 worldToLocal = localToWorld.GetOrthonormalInverse();

	Vec3  localStart     = worldToLocal.TransformPosition3D(query.m_start);
	Vec3  localDirection = worldToLocal.TransformDirection3D(query.m_direction);
	float localMaxLength = query.m_maxDistance;

	Ref<MeshResource> meshResource = m_mesh->GetMeshResource();

	hit.m_owner               = m_owner;
	(MathRaycastResult3D&)hit = RaycastVsAABB3D(localStart, localDirection, localMaxLength, meshResource->m_bounds);

	// Remember to transform the hit position and normal back into world space
	if (hit.m_didImpact)
	{
		hit.m_impactPos    = localToWorld.TransformPosition3D(hit.m_impactPos);
		hit.m_impactNormal = localToWorld.TransformDirection3D(hit.m_impactNormal).GetNormalized();
	}

	return hit;
}

RaycastResult3D MeshRaycastObject::IntersectRay(RaycastQuery3D const& query)
{
	RaycastResult3D hit;

	Matrix4x4 localToWorld = m_mesh->GetWorldTransform();
	Matrix4x4 worldToLocal = localToWorld.GetOrthonormalInverse();

	Vec3  localStart     = worldToLocal.TransformPosition3D(query.m_start);
	Vec3  localDirection = worldToLocal.TransformDirection3D(query.m_direction);
	float localMaxLength = query.m_maxDistance;

	Ref<MeshResource> meshResource = m_mesh->GetMeshResource();

	hit.m_owner = m_owner;
	for (const Triangle3& triangle : meshResource->m_triangles)
	{
		MathRaycastResult3D triangleHit = RaycastVsTriangle3D(localStart, localDirection, localMaxLength, triangle);
		if (triangleHit.m_didImpact && (!hit.m_didImpact || triangleHit.m_impactDist < hit.m_impactDist))
		{
			(MathRaycastResult3D&)hit = triangleHit;
		}
	}

	// Remember to transform the hit position and normal back into world space
	if (hit.m_didImpact)
	{
		hit.m_impactPos    = localToWorld.TransformPosition3D(hit.m_impactPos);
		hit.m_impactNormal = localToWorld.TransformDirection3D(hit.m_impactNormal).GetNormalized();
	}

	return hit;
}

void Mesh3D::BindMethods()
{
	ClassDatabase::BindMethod("SetMeshResource", &Mesh3D::SetMeshResource);
	ClassDatabase::BindMethod("GetMeshResource", &Mesh3D::GetMeshResource);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::Type::ObjectPtr,
			"mesh",
			PropertyInfo::Hint::ResourceType,
			MeshResource::GetStaticClassName(),
			PropertyInfo::UsageFlags::Default),
		"SetMeshResource",
		"GetMeshResource");
}

void Mesh3D::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr)
		{
			m_raycastObject          = new MeshRaycastObject();
			m_raycastObject->m_owner = GetHandle();
			m_raycastObject->m_mesh  = this;
			raycastSpace->AddObject(m_raycastObject);
		}
		break;
	}
	case NotificationType::ExitTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr)
		{
			raycastSpace->RemoveObject(m_raycastObject);
			delete m_raycastObject;
			m_raycastObject = nullptr;
		}
		break;
	}
	}
}

Mesh3D::~Mesh3D() { m_meshResource = nullptr; }

bool Mesh3D::IsEmpty() const { return m_meshResource == nullptr || m_meshResource->IsEmpty(); }

void Mesh3D::SetMeshResource(Variant meshResource)
{
	Ref<MeshResource> mesh = meshResource;
	if (!mesh.IsValid())
	{
		m_meshResource = nullptr;
		return;
	}

	m_meshResource = mesh;
}

Variant Mesh3D::GetMeshResource() const { return m_meshResource; }

RenderRequest Mesh3D::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_meshResource == nullptr || m_meshResource->IsEmpty())
	{
		return request;
	}

	request.m_pass                                  = RenderRequestPass::Opaque;
	request.m_modelToWorld                          = GetWorldTransform();
	request.m_tint                                  = Rgba8::White;
	request.m_vertexBuffer                          = m_meshResource->m_vertexBuffer;
	request.m_indexBuffer                           = m_meshResource->m_indexBuffer;
	request.m_textures[SurfaceTextureSlot::Diffuse] = m_meshResource->m_textureResources.size() > 0
														  ? m_meshResource->m_textureResources[0]->GetGPUTexture()
														  : nullptr;
	request.m_shader         = g_engine->m_renderer->CreateOrGetShader("res://Shaders/DefaultLit.hlsl");
	request.m_blendMode      = BlendMode::OPAQUE;
	request.m_depthMode      = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	request.m_samplerMode    = SamplerMode::POINT_CLAMP;
	return request;
}
