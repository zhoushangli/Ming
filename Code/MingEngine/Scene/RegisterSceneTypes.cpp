#include "MingEngine/Scene/RegisterSceneTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/MeshInstance3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/3D/VisualInstance3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/PackedSceneFormat.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Import/GLTFImporter.hpp"
#include "MingEngine/Scene/Import/ImageImporter.hpp"
#include "MingEngine/Scene/Import/OBJImporter.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"
#include "MingEngine/Scene/Resource/MeshResourceFormat.hpp"
#include "MingEngine/Scene/Resource/ShaderResource.hpp"
#include "MingEngine/Scene/Resource/ShaderResourceFormat.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"
#include "MingEngine/Scene/Resource/TextureResourceFormat.hpp"

#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"

namespace
{
	PackedSceneLoader *packedSceneLoader = new PackedSceneLoader();
	MeshResourceLoader *meshResourceLoader = new MeshResourceLoader();
	TextureResourceLoader *textureResourceLoader = new TextureResourceLoader();
	ShaderResourceLoader *shaderResourceLoader = new ShaderResourceLoader();

	PackedSceneSaver *packedSceneSaver = new PackedSceneSaver();
	MeshResourceSaver *meshResourceSaver = new MeshResourceSaver();
	TextureResourceSaver *textureResourceSaver = new TextureResourceSaver();

	OBJImporter *objImporter = new OBJImporter();
	GLTFImporter *gltfImporter = new GLTFImporter();
	ImageImporter *imageImporter = new ImageImporter();
} // namespace

#pragma region Scene

void RegisterSceneTypes()
{
	ClassDatabase::SetApiType(ApiType::Runtime);

	// Resource types
	ClassDatabase::RegisterClass<MeshResource>();
	ClassDatabase::RegisterClass<TextureResource>();
	ClassDatabase::RegisterClass<PackedScene>();
	ClassDatabase::RegisterClass<ShaderResource>();

	// Resource format types
	ClassDatabase::RegisterClass<PackedSceneLoader>();
	ClassDatabase::RegisterClass<MeshResourceLoader>();
	ClassDatabase::RegisterClass<TextureResourceLoader>();
	ClassDatabase::RegisterClass<ShaderResourceLoader>();

	ClassDatabase::RegisterClass<PackedSceneSaver>();
	ClassDatabase::RegisterClass<MeshResourceSaver>();
	ClassDatabase::RegisterClass<TextureResourceSaver>();

	ClassDatabase::RegisterClass<OBJImporter>();
	ClassDatabase::RegisterClass<GLTFImporter>();
	ClassDatabase::RegisterClass<ImageImporter>();

	// Resource format registration
	ResourceLoader::AddLoader(Ref<PackedSceneLoader>(packedSceneLoader));
	ResourceLoader::AddLoader(Ref<MeshResourceLoader>(meshResourceLoader));
	ResourceLoader::AddLoader(Ref<TextureResourceLoader>(textureResourceLoader));
	ResourceLoader::AddLoader(Ref<ShaderResourceLoader>(shaderResourceLoader));

	ResourceSaver::AddSaver(Ref<PackedSceneSaver>(packedSceneSaver));
	ResourceSaver::AddSaver(Ref<MeshResourceSaver>(meshResourceSaver));
	ResourceSaver::AddSaver(Ref<TextureResourceSaver>(textureResourceSaver));

	ResourceImporter::AddImporter(Ref<OBJImporter>(objImporter));
	ResourceImporter::AddImporter(Ref<GLTFImporter>(gltfImporter));
	ResourceImporter::AddImporter(Ref<ImageImporter>(imageImporter));

	// Scene types
	ClassDatabase::RegisterClass<Node>();
	ClassDatabase::RegisterClass<RaycastQuery3D>();
	ClassDatabase::RegisterClass<RaycastResult3D>();
	ClassDatabase::RegisterClass<RaycastSpace3D>();
	ClassDatabase::RegisterClass<Node3D>();
	ClassDatabase::RegisterClass<Camera3D>();
	ClassDatabase::RegisterAbstractClass<Light3D>();
	ClassDatabase::RegisterClass<DirectionalLight3D>();
	ClassDatabase::RegisterClass<OmniLight3D>();
	ClassDatabase::RegisterClass<SpotLight3D>();
	ClassDatabase::RegisterClass<Collider3D>(true);
	ClassDatabase::RegisterClass<AABBCollider3D>();
	ClassDatabase::RegisterClass<CapsuleCollider3D>();
	ClassDatabase::RegisterClass<CylinderZCollider3D>();
	ClassDatabase::RegisterClass<TriangleMeshCollider3D>();
	ClassDatabase::RegisterAbstractClass<VisualInstance3D>();
	ClassDatabase::RegisterClass<MeshInstance3D>();

	// Editor types
	ClassDatabase::SetApiType(ApiType::Editor);

	ClassDatabase::RegisterClass<EditorNode>();
	ClassDatabase::RegisterClass<EditorGizmos>();
	ClassDatabase::RegisterClass<EditorCamera>();
}

#pragma endregion
