#pragma once

#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Engine/Render/Rgba8.hpp"

struct JsonLoadOptions
{
	bool  m_flipWindingOrder = false;
	Rgba8 m_defaultColor     = Rgba8::White;
	float m_uniformScale     = 1.f;
};

// This loader is only for test "tiny glade" meshes
// Their models are all json files
class JsonLoader
{
public:
	static bool LoadFromFile(char const* filePath, MeshData& outMesh, JsonLoadOptions const& options = JsonLoadOptions());
};
