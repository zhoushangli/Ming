#pragma once

#include "MingEngine/Scene/Physics/Collider3D.hpp"

#include "MingEngine/Core/Render/Vertex.hpp"

#include <vector>

class TriangleMeshCollider3D : public Collider3D
{
	MCLASS(TriangleMeshCollider3D, Collider3D);

public:
	TriangleMeshCollider3D()           = default;
	~TriangleMeshCollider3D() override = default;

	void SetMesh(std::vector<Vertex> const& vertices);

	GameRaycastResult Raycast(RaycastInfo const& info) const override;

protected:
	void OnProcess(float deltaSeconds) override;

private:
	std::vector<Vertex> m_vertices;
};

