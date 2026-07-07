#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Triangle3.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

class Mesh3D;

class MeshRaycastObject : public RaycastObject
{
public:
	SceneRaycastResult3D IntersectBounds(RaycastQuery3D const& query) override;
	SceneRaycastResult3D IntersectRay(RaycastQuery3D const& query) override;

public:
	Mesh3D* m_mesh = nullptr;
};

class Mesh3D : public VisualizeInstance3D
{
	MCLASS(Mesh3D, VisualizeInstance3D);

public:
	Mesh3D() = default;
	~Mesh3D() override;

	bool IsEmpty() const;

	void    SetMeshResource(Variant meshResource);
	Variant GetMeshResource() const;

protected:
	static void BindMethods();

	void          OnNotification(int notification);
	RenderRequest SubmitRenderRequest() const override;

protected:
	Ref<MeshResource> m_meshResource;
	bool              m_useMaterialTextures = true;

	MeshRaycastObject* m_raycastObject;
};
