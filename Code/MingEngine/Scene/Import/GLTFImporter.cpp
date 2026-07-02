#include "MingEngine/Scene/Import/GLTFImporter.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

#include "ThirdParty/tinygltf/tiny_gltf_v3.h"

std::vector<std::string> GLTFImporter::GetSupportedExtensions() const { return { ".gltf", ".glb" }; }

bool GLTFImporter::Import(std::string const& sourceVirtualPath, std::string const& importVirtualPath)
{
	std::filesystem::path filePath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(sourceVirtualPath, filePath))
	{
		return false;
	}
	std::string const filePathString = filePath.string();

	tinygltf3::Model      model;
	tinygltf3::ErrorStack errors;
	tg3_error_code        parseResult = tinygltf3::parse_file(model, errors, filePathString.c_str());

	if (parseResult != TG3_OK)
	{
		return false;
	}

	if (model->meshes_count <= 0)
	{
		return false;
	}

	tg3_mesh const& mesh = model->meshes[0];
	if (mesh.primitives_count <= 0)
	{
		return false;
	}

	tg3_primitive const& primitive = mesh.primitives[0];
	if (primitive.mode != TG3_MODE_TRIANGLES)
	{
		return false;
	}
}