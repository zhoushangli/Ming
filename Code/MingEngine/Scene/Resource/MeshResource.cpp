#include "MingEngine/Scene/Resource/MeshResource.hpp"

#include "MingEngine/Scene/SceneCommon.hpp"
#include "MingEngine/Scene/Import/GLBLoader.hpp"
#include "MingEngine/Scene/Import/OBJLoader.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"
#include "MingEngine/Core/Render/VertexUtils.hpp"
#include "MingEngine/Engine/Render/IndexBuffer.hpp"
#include "MingEngine/Engine/Render/Renderer.hpp"
#include "MingEngine/Engine/Render/VertexBuffer.hpp"

#include <algorithm>
#include <cctype>

std::vector<MeshResource*> MeshResource::s_loadedMeshes;

namespace
{
std::string GetLowercaseExtension(std::string const& filePath)
{
	size_t const dotIndex = filePath.find_last_of('.');
	if (dotIndex == std::string::npos)
	{
		return "";
	}

	std::string extension = filePath.substr(dotIndex);
	std::transform(
		extension.begin(),
		extension.end(),
		extension.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); }
	);
	return extension;
}
} // namespace

void MeshData::Clear()
{
	m_vertices.clear();
	m_indices.clear();
	m_texture = nullptr;
}

bool MeshData::IsEmpty() const { return m_vertices.empty() || m_indices.empty(); }

MeshResource* MeshResource::CreateOrGetMesh(std::string const& modelFilePath, float scale)
{
	for (MeshResource* mesh : s_loadedMeshes)
	{
		if (mesh != nullptr && mesh->m_modelFilePath == modelFilePath && mesh->m_scale == scale)
		{
			return mesh;
		}
	}

	MeshResource* mesh = new MeshResource(modelFilePath, scale);
	s_loadedMeshes.push_back(mesh);
	return mesh;
}

MeshResource::MeshResource(std::string const& modelFilePath, float scale)
	: m_modelFilePath(modelFilePath), m_scale(scale)
{
	std::string const extension = GetLowercaseExtension(modelFilePath);
	if (extension == ".glb")
	{
		GLBLoadOptions options;
		options.m_uniformScale = scale;

		GLBLoader::LoadFromFile(modelFilePath.c_str(), m_meshData, options);
	}
	else
	{
		OBJLoadOptions options;
		options.m_uniformScale = scale;

		OBJLoader::LoadFromFile(modelFilePath.c_str(), m_meshData, options);
		TransformVertexArray3D(m_meshData.m_vertices, OBJToEngineTransform);
	}

	m_localBounds = GetVertexBounds3D(m_meshData.m_vertices);

	m_vertexBuffer = g_engine->m_renderer->CreateVertexBuffer(m_meshData.m_vertices);
	m_indexBuffer  = g_engine->m_renderer->CreateIndexBuffer(m_meshData.m_indices);
}

MeshResource::~MeshResource()
{
	delete m_vertexBuffer;
	m_vertexBuffer = nullptr;

	delete m_indexBuffer;
	m_indexBuffer = nullptr;
}

void MeshResource::ClearLoadedMeshes()
{
	for (MeshResource* mesh : s_loadedMeshes)
	{
		delete mesh;
	}

	s_loadedMeshes.clear();
}

bool MeshResource::IsEmpty() const { return m_meshData.IsEmpty(); }

