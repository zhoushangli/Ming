#include "MingEngine/Scene/3D/MeshInstance3D.hpp"

#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/BuiltinShaders.hpp"
#include "MingEngine/Engine/Render/DebugGizmos.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"

RaycastResult3D MeshRaycastObject::IntersectBounds(RaycastQuery3D const &query)
{
	RaycastResult3D hit;

	Matrix4x4 localToWorld = m_mesh->GetWorldTransform();
	Matrix4x4 worldToLocal = m_mesh->GetWorldInverseTransform();

	Vec3 localStart = worldToLocal.TransformPosition3D(query.m_start);
	Vec3 localDirection = worldToLocal.TransformDirection3D(query.m_direction);
	float localMaxLength = query.m_maxDistance;

	Ref<MeshResource> meshResource = m_mesh->GetMeshResource();

	if (!meshResource.IsValid())
	{
		return hit;
	}

	hit.m_owner = m_owner;
	hit = RaycastVsAABB3D(localStart, localDirection, localMaxLength, meshResource->m_bounds);

	// Remember to transform the hit position and normal back into world space
	if (hit.m_didImpact)
	{
		hit.m_impactPos = localToWorld.TransformPosition3D(hit.m_impactPos);
		hit.m_impactNormal = localToWorld.TransformDirection3D(hit.m_impactNormal).GetNormalized();
	}

	return hit;
}

RaycastResult3D MeshRaycastObject::IntersectRay(RaycastQuery3D const &query)
{
	RaycastResult3D hit;

	Matrix4x4 localToWorld = m_mesh->GetWorldTransform();
	Matrix4x4 worldToLocal = m_mesh->GetWorldInverseTransform();

	Vec3 localStart = worldToLocal.TransformPosition3D(query.m_start);
	Vec3 localDirection = worldToLocal.TransformDirection3D(query.m_direction);
	float localMaxLength = query.m_maxDistance;

	Ref<MeshResource> meshResource = m_mesh->GetMeshResource();

	if (!meshResource.IsValid())
	{
		return hit;
	}

	hit.m_owner = m_owner;
	for (const Triangle3 &triangle : meshResource->m_triangles)
	{
		MathRaycastResult3D triangleHit = RaycastVsTriangle3D(localStart, localDirection, localMaxLength, triangle);
		if (triangleHit.m_didImpact && (!hit.m_didImpact || triangleHit.m_impactDist < hit.m_impactDist))
		{
			hit = triangleHit;
		}
	}

	// Remember to transform the hit position and normal back into world space
	if (hit.m_didImpact)
	{
		hit.m_rayStartPos = localToWorld.TransformPosition3D(hit.m_rayStartPos);
		hit.m_rayFwdNormal = localToWorld.TransformDirection3D(hit.m_rayFwdNormal).GetNormalized();
		hit.m_impactPos = localToWorld.TransformPosition3D(hit.m_impactPos);
		hit.m_impactNormal = localToWorld.TransformDirection3D(hit.m_impactNormal).GetNormalized();
	}

	return hit;
}

void MeshInstance3D::BindMethods()
{
	ClassDatabase::BindMethod("SetMeshResource", &MeshInstance3D::SetMeshResource);
	ClassDatabase::BindMethod("GetMeshResource", &MeshInstance3D::GetMeshResource);

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

void MeshInstance3D::OnNotification(int notification)
{
	switch (static_cast<NotificationType>(notification))
	{
	case NotificationType::EnterTree:
	{
		RaycastSpace3D *raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr)
		{
			m_raycastObject = new MeshRaycastObject();
			m_raycastObject->m_owner = GetHandle();
			m_raycastObject->m_mesh = this;
			raycastSpace->AddObject(m_raycastObject);
		}
		break;
	}
	case NotificationType::ExitTree:
	{
		RaycastSpace3D *raycastSpace = GetSceneTree()->GetRaycastSpace();
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

MeshInstance3D::~MeshInstance3D() { m_meshResource = nullptr; }

bool MeshInstance3D::IsEmpty() const { return m_meshResource == nullptr || m_meshResource->IsEmpty(); }

void MeshInstance3D::SetMeshResource(Variant meshResource)
{
	Ref<MeshResource> mesh = meshResource;
	if (!mesh.IsValid())
	{
		m_meshResource = nullptr;
		return;
	}

	m_meshResource = mesh;
}

Variant MeshInstance3D::GetMeshResource() const { return m_meshResource; }

void MeshInstance3D::SetTint(Color tint) { m_tint = tint; }

Color MeshInstance3D::GetTint() const { return m_tint; }

RenderRequest MeshInstance3D::SubmitRenderRequest() const
{
	RenderRequest request;
	if (m_meshResource == nullptr || m_meshResource->IsEmpty())
	{
		return request;
	}

	request.m_pass = RenderRequestPass::Opaque;
	request.m_modelToWorld = GetWorldTransform();
	request.m_tint = m_tint;
	request.m_vertexBuffer = m_meshResource->m_vertexBuffer;
	request.m_indexBuffer = m_meshResource->m_indexBuffer;
	request.m_textures[SurfaceTextureSlot::Diffuse] = m_meshResource->m_textureResources.size() > 0
														  ? m_meshResource->m_textureResources[0]->GetGPUTexture()
														  : nullptr;
	Ref<ShaderResource> shaderResource =
		g_engine->m_renderer->GetBuiltinShaderResource("DefaultLit", BuiltinShaders::DefaultLit);
	request.m_shader = shaderResource.IsValid() ? shaderResource->GetShader() : nullptr;
	request.m_blendMode = BlendMode::OPAQUE;
	request.m_depthMode = DepthMode::READ_WRITE_LESS_EQUAL;
	request.m_rasterizerMode = RasterizerMode::SOLID_CULL_BACK;
	request.m_samplerMode = SamplerMode::POINT_CLAMP;
	return request;
}
