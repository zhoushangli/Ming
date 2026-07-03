#include "MingEngine/Scene/RegisterSceneTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/ResourceImporter.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptLoader.hpp"

#include "MingEngine/Engine/Application/SystemBase.hpp"
#include "MingEngine/Engine/Input/InputSystem.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/PackedSceneFormat.hpp"
#include "MingEngine/Scene/Import/OBJImporter.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"
#include "MingEngine/Scene/Resource/MeshResourceFormat.hpp"

namespace
{
ScriptLoader*       scriptLoader       = new ScriptLoader();
PackedSceneLoader*  packedSceneLoader  = new PackedSceneLoader();
PackedSceneSaver*   packedSceneSaver   = new PackedSceneSaver();
MeshResourceLoader* meshResourceLoader = new MeshResourceLoader();
MeshResourceSaver*  meshResourceSaver  = new MeshResourceSaver();
OBJImporter*        objImporter        = new OBJImporter();
} // namespace

void RegisterBaseTypes()
{
	ClassDatabase::RegisterRootClass<Object>();

	// Engine system types
	ClassDatabase::RegisterClass<SystemBase>(false, false);
	ClassDatabase::RegisterClass<InputSystem>(false, false);

	ClassDatabase::RegisterGlobalObject(g_engine->m_inputSystem);

	// Core types
	ClassDatabase::RegisterClass<RefCounted>(false);
	ClassDatabase::RegisterClass<Resource>(false);
	ClassDatabase::RegisterClass<Script>();
	ClassDatabase::RegisterClass<ResourceFormatImporter>(false);
	ClassDatabase::RegisterClass<ResourceFormatLoader>(false);
	ClassDatabase::RegisterClass<ResourceFormatSaver>(false);

	ClassDatabase::RegisterClass<ScriptLoader>();

	ResourceLoader::AddLoader(Ref<ScriptLoader>(scriptLoader));
}

namespace
{
void RegisterSceneResourceFormats()
{
	ClassDatabase::RegisterClass<PackedSceneLoader>();
	ClassDatabase::RegisterClass<PackedSceneSaver>();
	ClassDatabase::RegisterClass<MeshResourceLoader>();
	ClassDatabase::RegisterClass<MeshResourceSaver>();

	ResourceLoader::AddLoader(Ref<PackedSceneLoader>(packedSceneLoader));
	ResourceLoader::AddLoader(Ref<MeshResourceLoader>(meshResourceLoader));

	ResourceSaver::AddSaver(Ref<PackedSceneSaver>(packedSceneSaver));
	ResourceSaver::AddSaver(Ref<MeshResourceSaver>(meshResourceSaver));

	ResourceImporter::AddImporter(Ref<OBJImporter>(objImporter));
}
} // namespace

void RegisterSceneTypes()
{
	RegisterBaseTypes();
	RegisterSceneResourceFormats();

	// Scene types
	ClassDatabase::RegisterClass<Node>();
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
	ClassDatabase::RegisterClass<PackedScene>();
}
