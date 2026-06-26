#include "MingEngine/Scene/RegisterSceneTypes.hpp"

#include "MingEngine/Core/Object/ClassDatabase.hpp"
#include "MingEngine/Core/Object/RefCounted.hpp"
#include "MingEngine/Core/Object/Resource.hpp"
#include "MingEngine/Core/Object/ResourceLoader.hpp"
#include "MingEngine/Core/Object/ResourceSaver.hpp"
#include "MingEngine/Core/Object/Script.hpp"
#include "MingEngine/Core/Object/ScriptLoader.hpp"

#include "MingEngine/Scene/3D/Camera3D.hpp"
#include "MingEngine/Scene/3D/Light3D.hpp"
#include "MingEngine/Scene/3D/Node3D.hpp"
#include "MingEngine/Scene/Core/Node.hpp"
#include "MingEngine/Scene/Core/PackedScene.hpp"
#include "MingEngine/Scene/Core/PackedSceneLoader.hpp"
#include "MingEngine/Scene/Physics/AABBCollider3D.hpp"
#include "MingEngine/Scene/Physics/CapsuleCollider3D.hpp"
#include "MingEngine/Scene/Physics/CylinderZCollider3D.hpp"
#include "MingEngine/Scene/Physics/TriangleMeshCollider3D.hpp"

namespace
{
ScriptLoader*      scriptLoader      = new ScriptLoader();
PackedSceneLoader* packedSceneLoader = new PackedSceneLoader();
PackedSceneSaver*  packedSceneSaver  = new PackedSceneSaver();
} // namespace

void RegisterSceneTypes()
{
	ClassDatabase::RegisterRootClass<Object>();

	// Core types
	ClassDatabase::RegisterClass<RefCounted>(false);
	ClassDatabase::RegisterClass<Resource>(false);
	ClassDatabase::RegisterClass<Script>();
	ClassDatabase::RegisterClass<ResourceFormatLoader>(false);
	ClassDatabase::RegisterClass<ResourceFormatSaver>(false);

	ClassDatabase::RegisterClass<ScriptLoader>();
	ClassDatabase::RegisterClass<PackedSceneLoader>();
	ClassDatabase::RegisterClass<PackedSceneSaver>();
	ResourceLoader::AddLoader(Ref<ScriptLoader>(scriptLoader));
	ResourceLoader::AddLoader(Ref<PackedSceneLoader>(packedSceneLoader));
	ResourceSaver::AddSaver(Ref<PackedSceneSaver>(packedSceneSaver));

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