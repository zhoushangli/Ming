#include "MingEngine/Scene/3D/MeshInstance3D.hpp"

#include "MingEngine/Core/Math/RaycastUtils.hpp"
#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Engine/Render/RenderServer.hpp"
#include "MingEngine/Scene/Core/SceneTree.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

RaycastResult3D MeshRaycastObject::IntersectBounds(RaycastQuery3D const& query)
{
	RaycastResult3D hit;

	Matrix4x4 localToWorld = m_mesh->GetWorldTransform();
	Matrix4x4 worldToLocal = m_mesh->GetWorldInverseTransform();

	Vector3 localStart     = worldToLocal.TransformPosition3D(query.m_start);
	Vector3 localDirection = worldToLocal.TransformDirection3D(query.m_direction);
	float   localMaxLength = query.m_maxDistance;

	Ref<MeshResource> meshResource = m_mesh->GetMeshResource();

	if (!meshResource.IsValid())
	{
		return hit;
	}

	hit.m_owner = m_owner;
	hit         = RaycastVsAABB3D(localStart, localDirection, localMaxLength, meshResource->m_bounds);

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
	Matrix4x4 worldToLocal = m_mesh->GetWorldInverseTransform();

	Vector3 localStart     = worldToLocal.TransformPosition3D(query.m_start);
	Vector3 localDirection = worldToLocal.TransformDirection3D(query.m_direction);
	float   localMaxLength = query.m_maxDistance;

	Ref<MeshResource> meshResource = m_mesh->GetMeshResource();

	if (!meshResource.IsValid())
	{
		return hit;
	}

	hit.m_owner = m_owner;
	for (const Triangle3& triangle : meshResource->m_triangles)
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
		hit.m_rayStartPos  = localToWorld.TransformPosition3D(hit.m_rayStartPos);
		hit.m_rayFwdNormal = localToWorld.TransformDirection3D(hit.m_rayFwdNormal).GetNormalized();
		hit.m_impactPos    = localToWorld.TransformPosition3D(hit.m_impactPos);
		hit.m_impactNormal = localToWorld.TransformDirection3D(hit.m_impactNormal).GetNormalized();
	}

	return hit;
}

void MeshInstance3D::BindMethods()
{
	ClassDatabase::BindMethod("SetMeshResource", &MeshInstance3D::SetMeshResource, { "meshResource" });
	ClassDatabase::BindMethod("GetMeshResource", &MeshInstance3D::GetMeshResource, {});

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
	switch (notification)
	{
	case Notification_EnterTree:
	{
		RaycastSpace3D* raycastSpace = GetSceneTree()->GetRaycastSpace();
		if (raycastSpace != nullptr)
		{
			m_raycastObject          = new MeshRaycastObject();
			m_raycastObject->m_owner = GetObjectID();
			m_raycastObject->m_mesh  = this;
			raycastSpace->AddObject(m_raycastObject);
		}

		// The mesh may be assigned before the node enters the tree, so bind it here too.
		SyncMeshBase();
		break;
	}
	case Notification_ExitTree:
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

void MeshInstance3D::SyncMeshBase()
{
	RenderServer* server = g_engine != nullptr ? g_engine->m_renderServer : nullptr;
	if (server == nullptr || !m_instanceRID.IsValid())
	{
		return;
	}

	// An invalid Base RID keeps the instance type at None, so it produces no request.
	RID meshRID = m_meshResource.IsValid() ? m_meshResource->GetMeshRID() : RID::Invalid;
	server->InstanceSetBase(m_instanceRID, meshRID);
}

bool MeshInstance3D::IsEmpty() const { return m_meshResource == nullptr || m_meshResource->IsEmpty(); }

void MeshInstance3D::SetMeshResource(Variant meshResource)
{
	Ref<MeshResource> mesh = meshResource;
	m_meshResource         = mesh;

	// The resource owns its single Mesh RID, so the instance only points at it.
	SyncMeshBase();
}

Variant MeshInstance3D::GetMeshResource() const { return m_meshResource; }
