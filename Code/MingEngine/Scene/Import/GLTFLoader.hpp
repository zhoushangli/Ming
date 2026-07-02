#pragma once

#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Core/Render/Rgba8.hpp"

#include <string>

struct OBJHeader
{
	bool        m_isValid       = false;
	int         m_positionCount = 0;
	int         m_texCoordCount = 0;
	int         m_normalCount   = 0;
	int         m_faceCount     = 0;
	bool        m_hasTexCoords  = false;
	bool        m_hasNormals    = false;
	std::string m_errorMessage;
};

struct OBJLoadOptions
{
	bool  m_flipWindingOrder      = false;
	bool  m_invertVTexCoord       = false;
	bool  m_generateNormalsIfNone = true;
	bool  m_triangulateFaces      = true;
	Rgba8 m_defaultColor          = Rgba8::White;
	float m_uniformScale          = 1.f;
};

class OBJLoader
{
public:
	static bool LoadFromFile(
		char const*           filePath,
		MeshData&             outMesh,
		OBJHeader&            outHeader,
		OBJLoadOptions const& options = OBJLoadOptions()
	);

	static bool
	LoadFromFile(char const* filePath, MeshData& outMesh, OBJLoadOptions const& options = OBJLoadOptions());

private:
	static void GenerateNormalsIfNeeded(MeshData& mesh);
};

