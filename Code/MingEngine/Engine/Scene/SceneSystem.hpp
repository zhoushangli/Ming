#pragma once

#include "MingEngine/Core/Object/Object.hpp"

#include "MingEngine/Engine/Application/ProjectSettings.hpp"
#include "MingEngine/Core/Object/ObjectID.hpp"

class Node;
class SceneTree;

class SceneSystem : public Object
{
public:
	SceneSystem() = default;
	~SceneSystem();

	SceneSystem(SceneSystem const&) = delete;
	SceneSystem& operator=(SceneSystem const&) = delete;

	void Startup();
	void Shutdown();
	void Reset();

	SceneTree* GetSceneTree() const { return m_sceneTree; }
	Node* GetCurrentScene() const;

	// Prepare a replacement scene and defer its insertion until the scene tree flushes pending nodes.
	// e.g. ChangeScene("res://Level.tscn", error) preserves the current scene if loading fails.
	bool ChangeScene(VirtualPath const& path, std::string& error);
	bool ChangeScene(Node* scene, std::string& error);
	bool ReloadScene(std::string& error);
	void ClearScene();

	bool CreateAutoloads(std::vector<ProjectSettings::AutoLoad> const& entries, std::string& error);
	// Queue autoload destruction without replacing the current scene.
	// e.g. ClearAutoloads(); EndFrame(); removes the registered persistent nodes.
	void ClearAutoloads();

	void Process(float deltaSeconds);
	void PhysicsProcess(float fixedDeltaSeconds);
	void EndFrame();

private:
	SceneTree* m_sceneTree = nullptr;
	VirtualPath m_scenePath;
	ObjectID m_pathSceneID = ObjectID::Invalid;
	std::vector<ObjectID> m_autoloads;
};
