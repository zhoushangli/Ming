#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Triangle3.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Core/Render/Rgba8.hpp"
#include "MingEngine/Scene/3D/VisualInstance3D.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

class MeshInstance3D;

class MeshRaycastObject : public RaycastObject
{
public:
	RaycastResult3D IntersectBounds(RaycastQuery3D const &query) override;
	RaycastResult3D IntersectRay(RaycastQuery3D const &query) override;

public:
	MeshInstance3D *m_mesh = nullptr;
};

class MeshInstance3D : public VisualInstance3D
{
	MCLASS(MeshInstance3D, VisualInstance3D);

public:
	MeshInstance3D() = default;
	~MeshInstance3D() override;

	bool IsEmpty() const;

	void SetMeshResource(Variant meshResource);
	Variant GetMeshResource() const;
	void SetTint(Color tint);
	Color GetTint() const;

protected:
	static void BindMethods();

	void OnNotification(int notification);
	RenderRequest SubmitRenderRequest() const override;

protected:
	Ref<MeshResource> m_meshResource;
	bool m_useMaterialTextures = true;
	Color m_tint = Color::White;

	MeshRaycastObject *m_raycastObject;
};
