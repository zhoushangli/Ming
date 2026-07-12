#include "MingEngine/Scene/RegisterSceneTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Mesh3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/3D/VisualizeInstance3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/PackedSceneFormat.hpp"
#include "MingEngine/Scene/Core/RaycastSpace3D.hpp"
#include "MingEngine/Scene/Import/ImageImporter.hpp"
#include "MingEngine/Scene/Import/OBJImporter.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"
#include "MingEngine/Scene/Resource/MeshResourceFormat.hpp"
#include "MingEngine/Scene/Resource/TextureResource.hpp"
#include "MingEngine/Scene/Resource/TextureResourceFormat.hpp"

#if defined(MING_EDITOR)
#include "MingEngine/Editor/EditorCamera.hpp"
#include "MingEngine/Editor/EditorNode.hpp"
#include "MingEngine/Editor/Gizmos/EditorGizmos.hpp"
#endif

namespace
{
PackedSceneLoader*     packedSceneLoader     = new PackedSceneLoader();
PackedSceneSaver*      packedSceneSaver      = new PackedSceneSaver();
MeshResourceLoader*    meshResourceLoader    = new MeshResourceLoader();
MeshResourceSaver*     meshResourceSaver     = new MeshResourceSaver();
TextureResourceLoader* textureResourceLoader = new TextureResourceLoader();
TextureResourceSaver*  textureResourceSaver  = new TextureResourceSaver();
OBJImporter*           objImporter           = new OBJImporter();
ImageImporter*         imageImporter         = new ImageImporter();
} // namespace

#pragma region Scene

void RegisterSceneTypes()
{
	// Resource format types
	ClassDatabase::RegisterClass<PackedSceneLoader>();
	ClassDatabase::RegisterClass<PackedSceneSaver>();
	ClassDatabase::RegisterClass<MeshResourceLoader>();
	ClassDatabase::RegisterClass<MeshResourceSaver>();
	ClassDatabase::RegisterClass<TextureResourceLoader>();
	ClassDatabase::RegisterClass<TextureResourceSaver>();

	// Resource format registration
	ResourceLoader::AddLoader(Ref<PackedSceneLoader>(packedSceneLoader));
	ResourceLoader::AddLoader(Ref<MeshResourceLoader>(meshResourceLoader));
	ResourceLoader::AddLoader(Ref<TextureResourceLoader>(textureResourceLoader));

	ResourceSaver::AddSaver(Ref<PackedSceneSaver>(packedSceneSaver));
	ResourceSaver::AddSaver(Ref<MeshResourceSaver>(meshResourceSaver));
	ResourceSaver::AddSaver(Ref<TextureResourceSaver>(textureResourceSaver));

	ResourceImporter::AddImporter(Ref<OBJImporter>(objImporter));
	ResourceImporter::AddImporter(Ref<ImageImporter>(imageImporter));

	// Resource types
	ClassDatabase::RegisterClass<MeshResource>();
	ClassDatabase::RegisterClass<TextureResource>();
	ClassDatabase::RegisterClass<PackedScene>();

	// Scene types
	ClassDatabase::RegisterClass<Node>();
	ClassDatabase::RegisterClass<RaycastQuery3D>(false);
	ClassDatabase::RegisterClass<RaycastResult3D>(false, false);
	ClassDatabase::RegisterClass<RaycastSpace3D>(false, false);
	ClassDatabase::RegisterClass<Node3D>();
	ClassDatabase::RegisterClass<Camera3D>();
	ClassDatabase::RegisterClass<Light3D>(false);
	ClassDatabase::RegisterClass<DirectionalLight3D>();
	ClassDatabase::RegisterClass<PointLight3D>();
	ClassDatabase::RegisterClass<Collider3D>(false);
	ClassDatabase::RegisterClass<AABBCollider3D>();
	ClassDatabase::RegisterClass<CapsuleCollider3D>();
	ClassDatabase::RegisterClass<CylinderZCollider3D>();
	ClassDatabase::RegisterClass<TriangleMeshCollider3D>();
	ClassDatabase::RegisterClass<VisualizeInstance3D>(false);
	ClassDatabase::RegisterClass<Mesh3D>();

#if defined(MING_EDITOR)
	ClassDatabase::RegisterClass<EditorNode>(false);
	ClassDatabase::RegisterClass<EditorGizmos>(false);
	ClassDatabase::RegisterClass<EditorCamera>(false);
#endif
}

#pragma endregion
