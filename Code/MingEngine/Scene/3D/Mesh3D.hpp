#pragma once

#include "MingEngine/Core/Math/AABB3.hpp"
#include "MingEngine/Core/Math/EulerAngles.hpp"
#include "MingEngine/Core/Math/Matrix4x4.hpp"
#include "MingEngine/Core/Math/Vec3.hpp"
#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"
#include "MingEngine/Scene/Resource/MeshResource.hpp"

class Mesh3D : public VisualizeInstance3D
{
	MCLASS(Mesh3D, VisualizeInstance3D);

public:
	Mesh3D()           = default;
	~Mesh3D() override = default;

	AABB3 GetLocalBounds() const;
	bool  IsEmpty() const;
	void  SetUseMaterialTextures(bool useMaterialTextures);

	void    SetMesh(Variant meshResource);
	Variant GetMesh() const;

protected:
	static void BindMethods();

	void          OnProcess(float deltaSeconds) override;
	RenderRequest SubmitRenderRequest() const override;

protected:
	Ref<MeshResource> m_meshResource;
	bool              m_useMaterialTextures = true;
};
