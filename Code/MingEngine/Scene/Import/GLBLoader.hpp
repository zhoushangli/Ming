#pragma once

#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Engine/Render/Rgba8.hpp"

struct GLBLoadOptions
{
	Rgba8 m_defaultColor = Rgba8::White;
	float m_uniformScale = 1.f;
};

class GLBLoader
{
public:
	static bool LoadFromFile(char const* filePath, MeshData& outMesh, GLBLoadOptions const& options = GLBLoadOptions());
};
