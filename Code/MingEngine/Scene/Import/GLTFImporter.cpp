#include "MingEngine/Scene/Import/GLTFImporter.hpp"

#include "MingEngine/Engine/Application/Engine.hpp"

#include "ThirdParty/tinygltf/tiny_gltf_v3.h"

std::vector<std::string> GLTFImporter::GetSupportedExtensions() const { return { ".gltf", ".glb" }; }

std::string GLTFImporter::GetVisibleName() const { return "glTF as Mesh"; }

std::string GLTFImporter::GetImportedExtension() const { return "mesh"; }

Ref<Resource> GLTFImporter::Import(
	std::unordered_map<std::string, Variant> const& importOptions,
	std::string const&                              sourceVirtualPath)
{
	(void)importOptions;

	std::filesystem::path filePath;
	if (!g_engine->m_fileSystem->TryGetPhysicalPath(sourceVirtualPath, filePath))
	{
		return Ref<Resource>();
	}
	std::string const filePathString = filePath.string();

	tinygltf3::Model      model;
	tinygltf3::ErrorStack errors;
	tg3_error_code        parseResult = tinygltf3::parse_file(model, errors, filePathString.c_str());

	if (parseResult != TG3_OK)
	{
		return Ref<Resource>();
	}

	if (model->meshes_count <= 0)
	{
		return Ref<Resource>();
	}

	tg3_mesh const& mesh = model->meshes[0];
	if (mesh.primitives_count <= 0)
	{
		return Ref<Resource>();
	}

	tg3_primitive const& primitive = mesh.primitives[0];
	if (primitive.mode != TG3_MODE_TRIANGLES)
	{
		return Ref<Resource>();
	}

	return Ref<Resource>();
}
