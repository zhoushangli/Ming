#pragma once

#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"

#include "MingEngine/Engine/Math/AABB3.hpp"
#include "MingEngine/Engine/Math/EulerAngles.hpp"
#include "MingEngine/Engine/Math/Matrix4x4.hpp"
#include "MingEngine/Engine/Math/Vec3.hpp"

class MeshResource;

class Mesh3D : public VisualizeInstance3D
{
public:
	Mesh3D(std::string const& modelFilePath, float scale = 1.f);
	~Mesh3D() override;

	AABB3 GetLocalBounds() const;
	bool  IsEmpty() const;
	void  SetUseMaterialTextures(bool useMaterialTextures);

protected:
	void Update(float deltaSeconds) override;
	RenderRequest SubmitRenderRequest() const override;

protected:
	MeshResource* m_meshResource       = nullptr;
	bool          m_useMaterialTextures = true;
};
